#include "cleaner.h"

Cleaner::Cleaner(QSettings *settings, QObject *parent) : QObject(parent) {
    this->settings = settings;
    active = true;
} //endconstructor

void Cleaner::start() {
    while (active) {
        QDateTime tnow = QDateTime::currentDateTime();
        QStringList dirlist;

        QDir dir(settings->value("MotionDetector/savedir", "./").toString());
        dir.setFilter(QDir::Dirs | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);

        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            if (fileInfo.fileName() != "." && fileInfo.fileName() != "..") {
                QDateTime created = fileInfo.created();
                int diffraw = created.secsTo(tnow);
                if (diffraw >= 172800) {
                    dirlist << fileInfo.absoluteFilePath();
                } //endif
            } //endif
        } //endfor

        if (dirlist.size() > 0) {
            logger.log("Cleaner doing cleanup job:", Logger::OK);
        } //endif
        foreach (QString dir, dirlist) {
            logger.log("Deleting old folder "+dir, Logger::OK);
            removeDir(dir);
        } //endforeach

        usleep(3600000000u);
    } //endwhile
} //endfunction start

void Cleaner::stop() {
    active = false;
} //endfunction stop

void Cleaner::removeDir(QString dirName) {
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        foreach (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                removeDir(info.absoluteFilePath());
            } else {
                QFile::remove(info.absoluteFilePath());
            } //endif
        } //endforeach
        dir.rmdir(dirName);
    } //endif
} //endfunction removeDir
