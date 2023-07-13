#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QFile>

#include <logger.h>

#include <curl/curl.h>
#include <sys/stat.h>

class FtpClient : public QObject {
    Q_OBJECT
private:
    QSettings *settings;
    Logger logger;
    CURL *curl;
    QString syncfilepath;
    QString listfilepath;
    bool syncing;
    bool active;
    void sync();
    QStringList getRemoteDirContent(QString directory);
    QStringList getLocalDirContent(QString directory);
    void createRemoteDir(QString dir);
    void uploadFile(QString orig, QString dest);
    QStringList removeDuplicates(QStringList local, QStringList remote);
public:
    explicit FtpClient(QSettings *settings, QString syncfile, QString listfile, QObject *parent = 0);
public slots:
    void start();
    void stop();
};

#endif // FTPCLIENT_H
