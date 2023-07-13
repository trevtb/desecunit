#include "mjpgstreamer.h"

MjpgStreamer::MjpgStreamer(QSettings *settings, QObject* parent) : HttpRequestHandler(settings, parent) {
    this->settings = settings;
    iImgCounter = 0;
    iCurFrame = 0;
    logger.log("MJPG streamer started successfully.", Logger::OK);
} //endconstructor

void MjpgStreamer::imgReady(cv::Mat img) {
    QMutexLocker locker(&mut);
    matImg = img;
    if (iImgCounter > 10000) {
        iImgCounter = 3;
    } //endif
    iImgCounter++;
} //endfunction imgReady

cv::Mat MjpgStreamer::getFrame() {
    while(iCurFrame == iImgCounter) {
        usleep(1);
    } //endwhile
    QMutexLocker locker(&mut);
    cv::Mat retImg = matImg;
    iCurFrame = iImgCounter;
    return retImg;
} //endfunction getFrame

void MjpgStreamer::doAuthHead(HttpResponse& response) {
    response.setStatus(401, "Access denied");
    response.setHeader("WWW-Authenticate","Basic realm=\"DESEC Unit\"");
    response.write("");
} //endfunction doAuthHead

void MjpgStreamer::stop() {
    active = false;
} //endfunction stop

void MjpgStreamer::service(HttpRequest& request, HttpResponse& response, QString origin) {
    active = true;
    if (settings->value("MjpgStreamer/useauth", "0").toString() == "1") {
        QMultiMap<QByteArray,QByteArray> headers = request.getHeaderMap();
        QList<QByteArray> headerKeys = headers.keys();
        if (!headerKeys.contains("Authorization")) {
            doAuthHead(response);
            return;
        } else {
            QString concatenated = settings->value("MjpgStreamer/user", "admin").toString() + ":" + settings->value("MjpgStreamer/pass", "admin").toString();
            QString authdata("Basic " + concatenated.toLocal8Bit().toBase64());
            QString authval = headers.value("Authorization");
            if (authdata != authval) {
                logger.log("MJPG streamer authentication failed for client "+origin+".", Logger::WARNING);
                doAuthHead(response);
                return;
            } //endif
        } //endif
    } //endif

    QString path = request.getPath().data();

    if (path == "/stream") {
        logger.log("MJPG streamer streaming to "+origin+".", Logger::OK);
        response.setHeader("Cache-Control", "no-cache");
        response.setHeader("Pragma", "no-cache");
        response.setHeader("Expires", "Thu, 01 Dec 1994 16:00:00 GMT");
        response.setHeader("Connection", "close");
        response.setHeader("Content-Type", "multipart/x-mixed-replace; boundary=--jbound");
        response.write(" ");

        std::vector<uchar> imageDesVec;
        std::vector<int> compression_params;
        QByteArray rawImage;
        compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
        compression_params.push_back(70);
        cv::Mat cframe;
        while (active) {
            imageDesVec.clear();
            cframe = getFrame();
            cv::imencode(".jpg", cframe, imageDesVec, compression_params);
            rawImage = QByteArray((const char *) imageDesVec.data(), imageDesVec.size());
            response.write("--jbound\r\n", false, true);
            response.resetSentHeaders();
            response.setHeader("Content-Type", "image/jpeg");
            response.setHeader("Content-Length", rawImage.size());
            response.write(rawImage, false, true);
        } //endwhile
        response.write("\r\n", true, false);
    } else {
        response.setHeader("Content-Type", "text/html");
        response.write("<html><head></head><body><img src=\"/stream\" /></body></html>", true);
    } //endif
} //endfunction service
