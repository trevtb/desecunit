#-------------------------------------------------
#
# Project created by QtCreator 2015-05-02T01:33:59
#
#-------------------------------------------------

INCLUDEPATH += /usr/local/include
INCLUDEPATH += $$PWD/../../pi2/curl-7.42.1/include

QT       += core network
QT       -= gui

include($$PWD/httpserver/httpserver.pri)

TARGET = deseccunit
target.files = deseccunit
target.path = /home/pi/
INSTALLS += target
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    camio.cpp \
    runtime.cpp \
    motiondetector.cpp \
    mjpgstreamer.cpp \
    logger.cpp \
    ftpclient.cpp \
    smtphelper.cpp \
    alarm.cpp \
    cleaner.cpp

HEADERS += \
    camio.h \
    runtime.h \
    motiondetector.h \
    mjpgstreamer.h \
    logger.h \
    ftpclient.h \
    smtphelper.h \
    alarm.h \
    cleaner.h

unix:!macx: LIBS += -L$$PWD/../../pi2/opencv-2.4.11-arm/build/lib/ -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_objdetect -lopencv_ts -lopencv_video
INCLUDEPATH += $$PWD/../../pi2/opencv-2.4.11-arm/build/lib
DEPENDPATH += $$PWD/../../pi2/opencv-2.4.11-arm/build/lib

unix:!macx: LIBS += -L$$PWD/../../pi2/curl-7.42.1/lib/ -lcurl
INCLUDEPATH += $$PWD/../../pi2/curl-7.42.1/lib
