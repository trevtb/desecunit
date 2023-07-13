#include "smtphelper.h"

SmtpHelper::SmtpHelper(QSettings *settings, QDateTime atime, QObject *parent) : QObject(parent){
    this->settings = settings;
    this->atime = atime;
    this->sent = false;

    this->user = this->settings->value("MailClient/user", "").toString();
    this->pass = this->settings->value("MailClient/pass", "").toString();

    this->host = this->settings->value("MailClient/host", "").toString();
    this->port = this->settings->value("MailClient/port", 25).toInt();
    this->timeout = this->settings->value("MailClient/timeout", 120).toInt();
} //endconstructor

SmtpHelper::~SmtpHelper() {
    delete t;
    delete socket;
} //enddestructor

void SmtpHelper::start() {
    socket = new QSslSocket(this);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    QDateTime tnow = QDateTime::currentDateTime();
    QString message = "Das DESEC Sicherheitssystem hat am " + tnow.toString("dd.MM.yyyy") + " um " + tnow.toString("hh:mm:ss") +
            " Uhr eine Bewegung registriert. Die aufgenommenen Bilder befinden sich in dieser E-Mail.";
    QString subject = "DESEC: Bewegung erkannt";

    QStringList filelist = getFiles();

    sendMail(this->settings->value("MailClient/from", "").toString(), this->settings->value("MailClient/to", "").toString(), subject, message, filelist);
} //endfunction start

QStringList SmtpHelper::getFiles() {
    QStringList filelist;

    QString savedir = settings->value("MotionDetector/savedir", "./").toString();
    if (savedir[savedir.length()-1] != '/') {
        savedir += "/";
    } //endif

    QStringList folderlist;
    if (QDir(savedir + atime.toString("dd-MM-yy")).exists()) {
        folderlist << atime.toString("dd-MM-yy");
    } //endif
    QDateTime yesterday = atime.addDays(-1);
    if (QDir(savedir + yesterday.toString("dd-MM-yy")).exists()) {
        folderlist << yesterday.toString("dd-MM-yy");
    } //endif

    foreach (QString fp, folderlist) {
        QDir dir(savedir + fp);
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);

        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            if (fileInfo.fileName() != "." && fileInfo.fileName() != "..") {
                QDateTime created = fileInfo.created();
                int diffraw = atime.secsTo(created);
                if (diffraw < 0) {
                    diffraw = diffraw * -1;
                } //endif
                if (diffraw <= 5) {
                    filelist << QString(savedir + fp + "/" + fileInfo.fileName());
                } //endif
            } //endif
        } //endfor
    } //endforeach

    return filelist;
} //endfunction getFiles

void SmtpHelper::sendMail(const QString &from, const QString &to, const QString &subject, const QString &body, QStringList files) {
    message = "To: " + to + "\n";
    message.append("From: " + from + "\n");
    message.append("Subject: " + subject + "\n");

    //Let's intitiate multipart MIME with cutting boundary "frontier"
    message.append("MIME-Version: 1.0\n");
    message.append("Content-Type: multipart/mixed; boundary=frontier\n\n");

    message.append("--frontier\n");
    //message.append("Content-Type: text/html\n\n");  //Uncomment this for HTML formating, coment the line below
    message.append("Content-Type: text/plain\n\n");
    message.append(body);
    message.append("\n\n");

    if(!files.isEmpty()) {
        foreach(QString filePath, files) {
            QFile file(filePath);
            if(file.exists()) {
                if (!file.open(QIODevice::ReadOnly)) {
                    logger.log("Mail client couldn't open the file "+filePath+", sending failed.", Logger::ERROR);
                    return ;
                } //endif
                QByteArray bytes = file.readAll();
                message.append("--frontier\n");
                message.append("Content-Type: application/octet-stream\nContent-Disposition: attachment; filename="+ QFileInfo(file.fileName()).fileName() +";\nContent-Transfer-Encoding: base64\n\n");
                message.append(bytes.toBase64());
                message.append("\n");
            } //endif
        } //endforeach
    } //endif

    message.append("--frontier--\n");

    message.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n"));
    message.replace(QString::fromLatin1("\r\n.\r\n"),QString::fromLatin1("\r\n..\r\n"));

    this->from = from;
    rcpt = to;
    state = Init;
    socket->connectToHostEncrypted(host, port);
    if (!socket->waitForConnected(timeout)) {
        logger.log("Mail client could not connect to host: "+socket->errorString()+".", Logger::ERROR);
    } //endif

    t = new QTextStream(socket);

    while(state != Close) {
        if(!socket->waitForReadyRead()) {
            state = Close;
        } //endif
    } //endwhile
} //endfunction sendMail


void SmtpHelper::readyRead() {
    // SMTP is line-oriented
    QString responseLine;
    do {
        responseLine = socket->readLine();
        response += responseLine;
    } while ( socket->canReadLine() && responseLine[3] != ' ' );

    responseLine.truncate(3);

    if ( state == Init && responseLine == "220" ) {
        // banner was okay, let's go on
        *t << "EHLO localhost" <<"\r\n";
        t->flush();

        state = HandShake;
    } else if (state == HandShake && responseLine == "250") {
        if (socket->mode() == QSslSocket::UnencryptedMode) {
            socket->startClientEncryption();
            if (!socket->waitForEncrypted(timeout)) {
                state = Close;
            } //endif
        } //endif

        //Send EHLO once again but now encrypted
        *t << "EHLO localhost" << "\r\n";
        t->flush();
        state = Auth;
    } else if (state == Auth && responseLine == "250") {
        // Trying AUTH
        *t << "AUTH LOGIN" << "\r\n";
        t->flush();
        state = User;
    } else if (state == User && responseLine == "334") {
        //Trying User
        *t << QByteArray().append(user).toBase64()  << "\r\n";
        t->flush();

        state = Pass;
    } else if (state == Pass && responseLine == "334") {
        //Trying pass
        *t << QByteArray().append(pass).toBase64() << "\r\n";
        t->flush();

        state = Mail;
    } else if (state == Mail && responseLine == "235") {
        // HELO response was okay (well, it has to be)

        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        *t << "MAIL FROM:<" << from << ">\r\n";
        t->flush();
        state = Rcpt;
    } else if (state == Rcpt && responseLine == "250") {
        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        *t << "RCPT TO:<" << rcpt << ">\r\n"; //r
        t->flush();
        state = Data;
    } else if ( state == Data && responseLine == "250" ) {
        *t << "DATA\r\n";
        t->flush();
        state = Body;
    } else if ( state == Body && responseLine == "354" ) {
        *t << message << "\r\n.\r\n";
        t->flush();
        state = Quit;
    } else if ( state == Quit && responseLine == "250" ) {
        *t << "QUIT\r\n";
        t->flush();
        // here, we just close.
        state = Close;
        logger.log("Mail client successfully sent the message.", Logger::OK);
    } else if ( state == Close ) {
        deleteLater();
        return;
    } else {
        // something broke.
        logger.log("Mail client recieved an unrecognized answer from the SMTP server.", Logger::ERROR);
        state = Close;
    } //endif

    response = "";
} //endfunction readyRead
