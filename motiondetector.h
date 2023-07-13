#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <QObject>
#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <QSettings>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <logger.h>

using namespace std;
using namespace cv;

class MotionDetector: public QObject {
    Q_OBJECT

private:
    bool active;
    QMutex mut;
    QSettings *settings;
    int iImgCounter;
    int iCurFrame;
    Mat matImg;
    cv::Mat getFrame();
    inline int detectMotion(const Mat&, Mat&,
                     int, int, int, int, int, int,
                     Scalar & color);
    Logger logger;
public:
    explicit MotionDetector(QSettings *settings, QObject *parent = 0);
    void stop();
signals:
    void doAlarm(QDateTime altime);
public slots:
    void start();
    void imgReady(cv::Mat);
};

#endif // MOTIONDETECTOR_H
