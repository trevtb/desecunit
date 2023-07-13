#include "ftpclient.h"

FtpClient::FtpClient(QSettings *settings, QString syncfile, QString listfile, QObject *parent) : QObject(parent) {
    this->settings = settings;
    this->syncfilepath = syncfile;
    this->listfilepath = listfile;
    syncing = false;
} //endconstructor

static size_t write_flist(void *ptr, size_t size, size_t nmemb, void *stream) {
    FILE *writehere = (FILE *)stream;
    return fwrite(ptr, size, nmemb, writehere);
} //endfunction write_flist

static size_t noop_cb(void *, size_t size, size_t nmemb, void *) {
    return size * nmemb;
} //endfunction noop_cb

void FtpClient::stop() {
    active = false;
} //endfunction stop

void FtpClient::start() {
    active = true;
    logger.log("FTP client started successfully.", Logger::OK);

    // Cleanup list temp file
    if (0 == access(listfilepath.toStdString().c_str(), 0)) {
        QFile::remove(listfilepath);
    } //endif

    while (active) {
        if (0 == access(syncfilepath.toStdString().c_str(), 0) && !syncing) {
            syncing = true;
            QFile::remove(syncfilepath);
            sync();
        } //endif
        usleep(settings->value("FtpClient/interval", 60).toInt() * 1000000);
    } //endwhile
} //endfunction start

void FtpClient::sync() {
    logger.log("FTP client starting syncronization.", Logger::OK);

    QString localRoot = settings->value("MotionDetector/savedir", "./").toString();
    if (localRoot[localRoot.length()-1] != '/') {
        localRoot += "/";
    } //endif

    QString remoteRoot = settings->value("FtpClient/remotedir", "/").toString();
    if (remoteRoot[remoteRoot.length()-1] != '/') {
        remoteRoot += "/";
    } //endif

    QStringList locc = getLocalDirContent(localRoot);
    QStringList remc = getRemoteDirContent(remoteRoot);
    QStringList mkfolders = removeDuplicates(locc, remc);

    foreach (QString folder, mkfolders) {
        createRemoteDir(remoteRoot+folder);
    } //endforeach

    bool hascontent = false;
    foreach (QString folder, locc) {
        QStringList lcont = getLocalDirContent(localRoot + folder + "/");
        QStringList rcont = getRemoteDirContent(remoteRoot + folder);
        QStringList mkfiles = removeDuplicates(lcont, rcont);

        foreach (QString file, mkfiles) {
            if (!hascontent) {
                hascontent = true;
            } //endif
            QString orig = localRoot + folder + "/" + file;
            QString dest = remoteRoot + folder + "/" + file;
            logger.log("FTP client uploading file "+file+".", Logger::OK);
            uploadFile(orig, dest);
        } //endfor
    } //endforeach

    if (!hascontent) {
        logger.log("FTP client has nothing to do, everything is already in sync.", Logger::OK);
    } //endif
    logger.log("FTP client syncronization finished without errors.", Logger::OK);
    syncing = false;
} //endfunction sync

QStringList FtpClient::getRemoteDirContent(QString directory) {
    if (directory[directory.length()-1] != '/') {
        directory += "/";
    } //endif

    QStringList dirlist;
    QString url = "ftp://" + settings->value("FtpClient/host", "").toString() + directory;
    int port = settings->value("FtpClient/port", 21).toInt();
    QString loginstr = settings->value("FtpClient/user", "anonymous@example.com").toString() + ":" + settings->value("FtpClient/pass", "").toString();;

    CURL *curl;
    CURLcode res;
    FILE *ftplister;
    ftplister = fopen(listfilepath.toStdString().c_str(), "wb");

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, port);
        curl_easy_setopt(curl, CURLOPT_USERPWD, loginstr.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_flist);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, ftplister);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (CURLE_OK != res) {
            logger.log("FTP client failed to retrieve file list from server.", Logger::ERROR);
        } //endif
    } //endif

    fclose(ftplister);

    QFile listfile(listfilepath);
    if (listfile.open(QIODevice::ReadOnly)) {
        while(!listfile.atEnd()) {
            QString line = listfile.readLine().simplified();
            if (!line.isEmpty()) {
                dirlist.append(line);
            } //endif
        } //endwhile
    } //endif

    QFile::remove(listfilepath);

    return dirlist;
} //endfunction getRemoteDirContent

QStringList FtpClient::getLocalDirContent(QString directory) {
    QStringList retVal;

    QDir dir;
    if (!directory.isEmpty()) {
        dir = QDir(directory);
    } //endif
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName() != "." && fileInfo.fileName() != "..") {
            retVal << fileInfo.fileName();
        } //endif
    } //endfor

    return retVal;
} //endfunction getLocalDirContent

void FtpClient::createRemoteDir(QString dir) {
    QString url = "ftp://" + settings->value("FtpClient/host", "").toString();
    int port = settings->value("FtpClient/port", 21).toInt();
    QString loginstr = settings->value("FtpClient/user", "anonymous@example.com").toString() + ":" + settings->value("FtpClient/pass", "").toString();;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("MKD "+dir).toStdString().c_str());

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, port);
        curl_easy_setopt(curl, CURLOPT_USERPWD, loginstr.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
        curl_easy_setopt(curl, CURLOPT_QUOTE, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop_cb);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (CURLE_OK != res) {
            logger.log("FTP client failed to create remote directory.", Logger::ERROR);
        } //endif
    } //endif
} //endfunction createRemoteDir

void FtpClient::uploadFile(QString orig, QString dest) {
    QString url = "ftp://" + settings->value("FtpClient/host", "").toString();
    if (url[url.length()-1] != '/') {
        url += "/";
    } //endif
    url += dest;

    int port = settings->value("FtpClient/port", 21).toInt();
    QString loginstr = settings->value("FtpClient/user", "anonymous@example.com").toString() + ":" + settings->value("FtpClient/pass", "").toString();;

    QFileInfo fInfo(orig);
    QString filename = fInfo.fileName();

    CURL *curl;
    CURLcode res;

    FILE *hd_src;
    struct stat file_info;
    curl_off_t fsize;

    struct curl_slist *headerlist=NULL;
    headerlist = curl_slist_append(headerlist, ("RNFR "+ filename).toStdString().c_str());

    if (stat(orig.toStdString().c_str(), &file_info)) {
        return;
    } //endif
    fsize = (curl_off_t)file_info.st_size;
    hd_src = fopen(orig.toStdString().c_str(), "rb");

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, port);
        curl_easy_setopt(curl, CURLOPT_USERPWD, loginstr.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop_cb);

        res = curl_easy_perform(curl);
        curl_slist_free_all(headerlist);
        curl_easy_cleanup(curl);

        if (CURLE_OK != res) {
            logger.log("FTP client failed to create remote directory.", Logger::ERROR);
        } //endif
    } //endif

    fclose(hd_src);
    curl_global_cleanup();
} //endfunction uploadFile

QStringList FtpClient::removeDuplicates(QStringList local, QStringList remote) {
    QStringList tmp = QStringList(local);
    for (int i=0; i<tmp.size(); i++) {
        for (int j=0; j<remote.size(); j++) {
            if (tmp.at(i) == remote.at(j)) {
                tmp[i] = "";
            } //endif
        } //endfor
    } //endfor

    QStringList retVal;
    foreach (QString entry, tmp) {
        if (!entry.isEmpty()) {
            retVal << entry;
        } //endif
    } //endforeach

    return retVal;
} //endfunction removeDuplicates
