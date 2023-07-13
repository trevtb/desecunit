#ifndef RUNTIME_H
#define RUNTIME_H

#include <QObject>
#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include <QSettings>

#include "httplistener.h"

#include <camio.h>
#include <motiondetector.h>
#include <ftpclient.h>
#include <logger.h>
#include <alarm.h>
#include <cleaner.h>

class Runtime : public QObject {
    Q_OBJECT
private:
    QSettings *settings;
    QString syncfilepath;
    QString alarmlock;
    QDateTime lastAlarm;
    QThread thrMD;
    QThread thrCIO;
    QThread thrHTTP;
    QThread thrFTP;
    QThread thrCleaner;
    QThread thrAlarm;
    Logger logger;
    CamIO *cio;
    MotionDetector *md;
    HttpListener *listener;
    FtpClient *ftp;
    Cleaner *cleaner;
    Alarm *alarm;
public:
    explicit Runtime(QObject *parent = 0);
    void start();
    void stop(); 
    static const QString ALARMLOCK;
    static const QString SYNCLOCK;
    static const QString FTPLIST;
public slots:
    void setImage(cv::Mat);
    void doAlarm(QDateTime tnow);
signals:
    void imgReady(cv::Mat);
    void fireAlarm(QDateTime);
};

#endif // RUNTIME_H
