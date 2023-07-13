#ifndef ALARM_H
#define ALARM_H

#include <QObject>
#include <QDateTime>
#include <QSettings>
#include <QFile>
#include <QMutex>

#include <smtphelper.h>

class Alarm : public QObject {
    Q_OBJECT
private:
    bool active;
    bool alarm;
    Logger logger;
    QMutex mutex;
    QString lockfile;
    QSettings *settings;
    QDateTime lastMail;
    QDateTime currentAlarm;
    bool isScheduled(QDateTime now);
public:
    explicit Alarm(QSettings *settings, QString lockfile, QObject *parent = 0);
signals:

public slots:
    void start();
    void stop();
    void fireAlarm(QDateTime altime);
};

#endif // ALARM_H
