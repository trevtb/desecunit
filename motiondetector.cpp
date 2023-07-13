#include "motiondetector.h"

MotionDetector::MotionDetector(QSettings *settings, QObject *parent) : QObject(parent) {
    active = true;
    iImgCounter = 0;
    iCurFrame = 0;
    this->settings = settings;
} //endconstructor

void MotionDetector::stop() {
    active = false;
} //endfunction stop

void MotionDetector::imgReady(cv::Mat img) {
    QMutexLocker locker(&mut);
    matImg = img;
    if (iImgCounter > 10000) {
        iImgCounter = 3;
    } //endif
    iImgCounter++;
} //endfunction imgReady

Mat MotionDetector::getFrame() {
    while(iCurFrame == iImgCounter) {
        usleep(1);
    } //endwhile
    QMutexLocker locker(&mut);
    Mat retImg = matImg;
    iCurFrame = iImgCounter;
    return retImg;
} //endfunction getFrame

inline int MotionDetector::detectMotion(const Mat & motion, Mat & result,
                 int x_start, int x_stop, int y_start, int y_stop,
                 int max_deviation, int thresh,
                 Scalar & color) {
    // calculate the standard deviation
    Scalar mean, stddev;
    meanStdDev(motion, mean, stddev);
    // if not to much changes then the motion is real (neglect agressive snow, temporary sunlight)
    if(stddev[0] < max_deviation) {
        int number_of_changes = 0;
        int min_x = motion.cols, max_x = 0;
        int min_y = motion.rows, max_y = 0;
        // loop over image and detect changes
        for (int j = y_start; j < y_stop; j+=2) { // height
            for (int i = x_start; i < x_stop; i+=2) { // width
                // check if at pixel (j,i) intensity is equal to 255
                // this means that the pixel is different in the sequence
                // of images (prev_frame, current_frame, next_frame)
                if (static_cast<int>(motion.at<uchar>(j,i)) >= thresh) {
                    number_of_changes++;
                    if(min_x>i) min_x = i;
                    if(max_x<i) max_x = i;
                    if(min_y>j) min_y = j;
                    if(max_y<j) max_y = j;
                } //endif
            } //endfor
        } //endfor
        if (number_of_changes) {
            //check if not out of bounds
            if(min_x-10 > 0) min_x -= 10;
            if(min_y-10 > 0) min_y -= 10;
            if(max_x+10 < result.cols-1) max_x += 10;
            if(max_y+10 < result.rows-1) max_y += 10;
            // draw rectangle round the changed pixel
            Point x(min_x,min_y);
            Point y(max_x,max_y);
            Rect rect(x,y);
            rectangle(result,rect,color,1);
        } //endif
        return number_of_changes;
    } //endif
    return 0;
} //endfunction detectMotion

void MotionDetector::start() {
    logger.log("Motion detector started successfully.", Logger::OK);

    QString dirpath = settings->value("MotionDetector/savedir", "/etc/desecunit/motion").toString();
    if (dirpath[dirpath.length()-1] != '/') {
        dirpath += "/";
    } //endif

    bool alarm = false;
    if (settings->value("Alarm/enabled", 0).toInt() == 1) {
        alarm = true;
    } //endif

    Mat result, result_cropped;
    Mat prev_frame = result = getFrame();
    Mat current_frame = getFrame();
    Mat next_frame = getFrame();

    cvtColor(current_frame, current_frame, CV_RGB2GRAY);
    cvtColor(prev_frame, prev_frame, CV_RGB2GRAY);
    cvtColor(next_frame, next_frame, CV_RGB2GRAY);

    Mat d1, d2, motion;
    int number_of_changes, number_of_sequence = 0;
    Scalar color(0,0,255); // red
    int x_start = 0, x_stop = current_frame.cols;
    int y_start = 0, y_stop = current_frame.rows;

    int there_is_motion = settings->value("MotionDetector/threshold", 5).toInt();
    int max_deviation = settings->value("MotionDetector/deviation", 20).toInt();
    int thresh = settings->value("MotionDetector/pixthresh", 32).toInt();

    Mat kernel_ero = getStructuringElement(MORPH_RECT, Size(2,2));

    int cc = 0;
    while (active) {
        prev_frame = current_frame;
        current_frame = next_frame;
        next_frame = getFrame();
        result = next_frame;
        cvtColor(next_frame, next_frame, CV_RGB2GRAY);
        absdiff(prev_frame, next_frame, d1);
        absdiff(next_frame, current_frame, d2);
        bitwise_and(d1, d2, motion);
        erode(motion, motion, kernel_ero);

        number_of_changes = detectMotion(motion, result, x_start, x_stop, y_start, y_stop, max_deviation, thresh, color);
        if (number_of_changes >= there_is_motion) {
           if(number_of_sequence>0) {
               QDateTime tnow = QDateTime::currentDateTime();
               logger.log("Motion Detector detected motion.", Logger::OK, tnow.toString("dd-MM-yy hh:mm:"));

               if (alarm) {
                   QDir dir(dirpath + tnow.toString("dd-MM-yy"));
                   if (!dir.exists()) {
                       dir.mkpath(".");
                   } //endif
                   imwrite(QString(dirpath + tnow.toString("dd-MM-yy") + "/" +
                                                  tnow.toString("hh-mm-ss-zzz") + ".jpg").toStdString().c_str(), result);
                   emit doAlarm(tnow);
               } //endif

           } //endif
           number_of_sequence++;
        } else {
           number_of_sequence = 0;
           usleep(300000);
        } //endif
        cc = cc+1;
    } //endwhile
} //endfunction start
