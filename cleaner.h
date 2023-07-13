#ifndef CLEANER_H
#define CLEANER_H

#include <QObject>
#include <QSettings>
#include <QDir>
#include <QDateTime>
#include <QStringList>

#include <logger.h>

class Cleaner : public QObject {
    Q_OBJECT
private:
    QSettings *settings;
    bool active;
    Logger logger;
    void removeDir(QString dirName);
public:
    explicit Cleaner(QSettings *settings, QObject *parent = 0);
public slots:
    void start();
    void stop();
};

#endif // CLEANER_H
