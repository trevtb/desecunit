#ifndef MJPGSTREAMER_H
#define MJPGSTREAMER_H

#include <QMutex>
#include <QDateTime>
#include <QSettings>

#include <signal.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "httprequesthandler.h"

#include <logger.h>

class MjpgStreamer : public HttpRequestHandler {
    Q_OBJECT
private:
    bool active;
    QSettings *settings;
    QMutex mut;
    int iImgCounter;
    int iCurFrame;
    cv::Mat matImg;
    cv::Mat getFrame();
    Logger logger;
    void doAuthHead(HttpResponse& response);
public:
    MjpgStreamer(QSettings *settings, QObject* parent=0);
    void stop();
    void service(HttpRequest& request, HttpResponse& response, QString origin = "");
public slots:
    void imgReady(cv::Mat);
};

#endif // MJPGSTREAMER_H
