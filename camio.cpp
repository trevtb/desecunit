#include "camio.h"

CamIO::CamIO(QSettings *settings, QObject *parent) : QObject(parent) {
    this->settings = settings;
    active = true;
    cam = new VideoCapture(-1);
    if (!cam->isOpened()) {
        camavailable = false;
    } else {
        camavailable = true;
    } //endif
    QString sResolution = this->settings->value("Camera/resolution", "320x240").toString();
    QStringList slResolution = sResolution.split("x");
    cam->set(CV_CAP_PROP_FRAME_WIDTH, slResolution[0].toDouble());
    cam->set(CV_CAP_PROP_FRAME_HEIGHT, slResolution[1].toDouble());
} //endconstructor

void CamIO::stop() {
    active = false;
} //endfunction stop

bool CamIO::camAvailable() {
    return camavailable;
} //endfunction camAvailable

Mat CamIO::getFrame() {
    Mat frame;
    cam->read(frame);
    return frame;
} //endfunction getFrame

void CamIO::start() {
    while (active) {
        emit setImage(getFrame());
    } //endwhile
    cam->release();
} //endfunction start
