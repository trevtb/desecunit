#include "runtime.h"

const QString Runtime::ALARMLOCK = QString("/tmp/desecalarm.lock");
const QString Runtime::SYNCLOCK = QString("/tmp/desecsync.lock");
const QString Runtime::FTPLIST = QString("/tmp/desecftplist.tmp");

Runtime::Runtime(QObject *parent) : QObject(parent) {
    this->settings = new QSettings("/home/pi/settings.ini", QSettings::IniFormat);
} //endconstructor

void Runtime::start() {
    logger.log("Starting DESEC unit.", Logger::OK);

    // Cleanup alarm lock file
    if (0 == access(ALARMLOCK.toStdString().c_str(), 0)) {
        QFile::remove(ALARMLOCK);
    } //endif

    // Create the cleaner daemon
    cleaner = new Cleaner(settings);
    cleaner->moveToThread(&thrCleaner);
    connect(&thrCleaner, SIGNAL(started()), cleaner, SLOT(start()));
    thrCleaner.start();

    // Create the camera daemon
    cio = new CamIO(settings);
    if (!cio->camAvailable()) {
        logger.log("No camera found, exiting. (C01)", Logger::ERROR);
        exit(0);
    } //endif
    qRegisterMetaType< cv::Mat >("cv::Mat");
    cio->moveToThread(&thrCIO);
    connect(&thrCIO, SIGNAL(started()), cio, SLOT(start()));
    connect(cio, SIGNAL(setImage(cv::Mat)), this, SLOT(setImage(cv::Mat)), Qt::DirectConnection);
    thrCIO.start();

    if (settings->value("MotionDetector/enabled", 1).toInt() == 1) {
        md = new MotionDetector(settings);
        connect(md, SIGNAL(doAlarm(QDateTime)), this, SLOT(doAlarm(QDateTime)));
        md->moveToThread(&thrMD);
        connect(&thrMD, SIGNAL(started()), md, SLOT(start()));
        connect(this, SIGNAL(imgReady(cv::Mat)), md, SLOT(imgReady(cv::Mat)), Qt::DirectConnection);
        thrMD.start();
    } //endif

    if (settings->value("MotionDetector/enabled", 1).toInt() == 1 && settings->value("Alarm/enabled", 1).toInt() == 1) {
        alarm = new Alarm(settings, ALARMLOCK);
        connect(this, SIGNAL(fireAlarm(QDateTime)), alarm, SLOT(fireAlarm(QDateTime)), Qt::DirectConnection);
        alarm->moveToThread(&thrAlarm);
        connect(&thrAlarm, SIGNAL(started()), alarm, SLOT(start()));
        thrAlarm.start();
    } //endif

    if (settings->value("MjpgStreamer/enabled", "1").toString() == "1") {
        listener = new HttpListener(settings, this);
    } //endif

    if (settings->value("FtpClient/enabled", "0").toString() == "1") {
        // Enable sync state in case we missed syncing due to an unexpected program termination
        if (-1 == access(SYNCLOCK.toStdString().c_str(), 0)) {
            QFile sfp(SYNCLOCK);
            if (sfp.open(QIODevice::ReadWrite)) {
                sfp.close();
            } //endif
        } //endif

        ftp = new FtpClient(settings, SYNCLOCK, FTPLIST);
        ftp->moveToThread(&thrFTP);
        connect(&thrFTP, SIGNAL(started()), ftp, SLOT(start()));
        thrFTP.start();
    } //endif
} //endfunction start

void Runtime::stop() {
    if (settings->value("MjpgStreamer/enabled", "1").toString() == "1") {
        listener->stop();
    } //endif
    if (settings->value("MotionDetector/enabled", "1").toString() == "1") {
       md->stop();
    } //endif
    if (settings->value("MotionDetector/enabled", 1).toInt() == 1 && settings->value("Alarm/enabled", 1).toInt() == 1) {
        alarm->stop();
    } //endif
    cio->stop(); 
    if (settings->value("FtpClient/enabled", "1").toString() == "1") {
        ftp->stop();
    } //endif
} //endfunction stop

void Runtime::setImage(cv::Mat img) {
    emit imgReady(img);
} //endfunction setImage

void Runtime::doAlarm(QDateTime altime) {
    if (-1 == access(SYNCLOCK.toStdString().c_str(), 0)) {
        QFile sfp(SYNCLOCK);
        if (sfp.open(QIODevice::ReadWrite)) {
            sfp.close();
        } //endif
    } //endif

    if (-1 == access(ALARMLOCK.toStdString().c_str(), 0)) {
        QFile lock(ALARMLOCK);
        if (lock.open(QIODevice::ReadWrite)) {
            lock.close();
        } //endif
        emit fireAlarm(altime);
    } //endif
} //endfunction doAlarm
