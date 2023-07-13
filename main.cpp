#include <QCoreApplication>
#include <QDebug>

#include <signal.h>

#include <logger.h>
#include <runtime.h>

Runtime *myrun;

static void signalHandler (int signo) {
    // Cleanup alarm lock file
    if (0 == access(Runtime::ALARMLOCK.toStdString().c_str(), 0)) {
        QFile::remove(Runtime::ALARMLOCK);
    } //endif
    // Cleanup sync lock file
    if (0 == access(Runtime::SYNCLOCK.toStdString().c_str(), 0)) {
        QFile::remove(Runtime::SYNCLOCK);
    } //endif
    // Cleanup ftp list file
    if (0 == access(Runtime::FTPLIST.toStdString().c_str(), 0)) {
        QFile::remove(Runtime::FTPLIST);
    } //endif

    Logger *logger = new Logger();

    if (signo == SIGINT) {
        logger->log("DESEC unit recieved SIGINT, exiting.", Logger::OK);
        exit(0);
    } else if (signo == SIGTERM) {
        logger->log("DESEC unit recieved SIGTERM, exiting.", Logger::OK);
        exit(0);
    } else if (signo == SIGHUP) {
        logger->log("DESEC unit recieved SIGHUP, restarting.", Logger::OK);
        myrun->stop();
        usleep(1000000);
        myrun->deleteLater();
        myrun = new Runtime();
        myrun->start();
    } //endif
} //endfunction signalHandler

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);

    myrun = new Runtime();
    myrun->start();

    return a.exec();
} //endfunction main
