#ifndef CAMIO_H
#define CAMIO_H

#include <QObject>
#include <QStringList>
#include <QSettings>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

class CamIO: public QObject {
    Q_OBJECT
private:
    bool active;
    VideoCapture *cam;
    bool camavailable;
    QSettings *settings;
    double vwidth;
    double vheight;
    Mat getFrame();
public:
    explicit CamIO(QSettings *settings, QObject *parent = 0);
    bool camAvailable();
    void stop();
public slots:
    void start();
signals:
    void setImage(cv::Mat);
};

#endif // CAMIO_H
