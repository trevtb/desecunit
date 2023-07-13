#ifndef SMTPHELPER_H
#define SMTPHELPER_H

#include <QObject>
#include <QStringList>
#include <QWaitCondition>
#include <QMutex>
#include <QSettings>
#include <QDateTime>
#include <QDir>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QSslSocket>
#include <QTextStream>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>

#include <logger.h>

class SmtpHelper : public QObject {
    Q_OBJECT

private:
    QSettings *settings;
    QStringList getFiles();
    QDateTime atime;
    int timeout;
    QString message;
    QTextStream *t;
    QSslSocket *socket;
    QString from;
    QString rcpt;
    QString response;
    QString user;
    QString pass;
    QString host;
    bool sent;
    int port;
    enum states{Tls, HandShake ,Auth,User,Pass,Rcpt,Mail,Data,Init,Body,Quit,Close};
    int state;
    void sendMail(const QString &from, const QString &to,
                       const QString &subject, const QString &body,
                       QStringList files = QStringList());
signals:
    void status(const QString &);
private slots:
    void readyRead();
public:
    Logger logger;
    explicit SmtpHelper(QSettings *settings, QDateTime atime, QObject *parent = 0);
    ~SmtpHelper();
    void start();
};

#endif // SMTPHELPER_H
