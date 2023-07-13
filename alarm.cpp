#include "alarm.h"

Alarm::Alarm(QSettings *settings, QString lockfile, QObject *parent) : QObject(parent) {
    this->settings = settings;
    this->lockfile = lockfile;
    this->alarm = false;
} //endconstructor

void Alarm::start() {
    active = true;

    while (active) {
        mutex.lock();
        bool enabled = alarm;
        mutex.unlock();
        if (enabled) {
            bool scheduled = false;
            if (isScheduled(currentAlarm)) {
                scheduled = true;
            } //endif

            if (settings->value("MailClient/enabled", 0).toInt() == 1 && scheduled) {
                int differ = lastMail.secsTo(currentAlarm);
                if (differ >= settings->value("MailClient/senddelay", 300).toInt()) {
                    usleep(3000000);
                    logger.log("Alarm triggered, sending notification mail.", Logger::OK);
                    SmtpHelper *smtp = new SmtpHelper(this->settings, currentAlarm);
                    smtp->start();
                    delete smtp;
                    lastMail = currentAlarm;
                } //endif
            } //endif
            alarm = false;
            QFile::remove(lockfile);
        } //endif
        usleep(100000);
    } //endwhile
} //endfunction start

void Alarm::stop() {
    active = false;
} //endfunction stop

void Alarm::fireAlarm(QDateTime altime) {
    if (!alarm) {
        this->currentAlarm = altime;
        mutex.lock();
        alarm = true;
        mutex.unlock();
    } //endif
} //endfunction fireAlarm

bool Alarm::isScheduled(QDateTime now) {
    QTime ntime = now.time();
    ntime.setHMS(ntime.hour(), ntime.minute(), 0);
    now.setTime(ntime);
    QDate today = now.date();
    QLocale locale  = QLocale(QLocale::English, QLocale::UnitedKingdom);
    QString dayofweek = (locale.toString(today, "dddd")).toLower();
    QStringList timestrs;
    QVariant value = settings->value("Alarm/"+dayofweek, "");
    if (value.type() == QVariant::StringList) {
      timestrs = value.toStringList();
    } else if (value.type() == QVariant::String) {
      timestrs << value.toString();
    } //endif

    if (timestrs.isEmpty()) {
        return false;
    } //endif

    foreach (QString timestr, timestrs) {
        QStringList timespan = timestr.split("-");
        if (timespan.size() != 2) {
            logger.log("Alarm scheduler detected wrong time format for "+dayofweek+".", Logger::ERROR);
            return false;
        } //endif
        QDateTime begin = QDateTime::currentDateTime();
        QDateTime end = QDateTime::currentDateTime();
        for (int i=0; i<timespan.size(); i++) {
            QString time = timespan.at(i);
            QStringList hrmin = time.split(":");
            if (hrmin.size() != 2) {
                logger.log("Alarm scheduler detected wrong time format for "+dayofweek+".", Logger::ERROR);
                return false;
            } //endif
            QTime ctime;
            ctime.setHMS(hrmin.at(0).toInt(), hrmin.at(1).toInt(), 0);
            if (i == 0) {
                begin.setTime(ctime);
            } else {
                end.setTime(ctime);
            } //endif
        } //endfor
        int diffbegin = begin.secsTo(now);
        int diffend = now.secsTo(end);
        if (diffbegin >= 0 && diffend >= 0) {
            return true;
        } //endif
    } //endforeach

    return false;
} //endfunction isScheduled

