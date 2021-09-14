#-------------------------------------------------
#
# Project created by QtCreator 2021-07-31T16:34:12
#
#-------------------------------------------------

QT       += core gui concurrent xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = v4l2camera
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    imageprocess.cpp \
    main.cpp \
    mainwindow.cpp \
    camera.cpp \
    cameralabel.cpp \
    v4l2device.cpp \
    v4l2param.cpp

HEADERS += \
    imageprocess.h \
    logger.hpp \
    mainwindow.h \
    camera.h \
    cameralabel.h \
    reallocator.hpp \
    v4l2device.h \
    v4l2param.h

FORMS += \
        mainwindow.ui
#libyuv
LIBYUV_PATH = /home/eigen/MySpace/3rdPartyLibrary/libyuv
INCLUDEPATH += $$LIBYUV_PATH/include
LIBS += -L$$LIBYUV_PATH/lib -lyuv -ljpeg
#opencv
OPENCV_PATH = /home/eigen/MySpace/3rdPartyLibrary/opencv45
INCLUDEPATH += $$OPENCV_PATH/include/opencv4
LIBS += -L$$OPENCV_PATH/lib -lopencv_calib3d \
                            -lopencv_core \
                            -lopencv_dnn \
                            -lopencv_features2d \
                            -lopencv_flann \
                            -lopencv_gapi \
                            -lopencv_highgui \
                            -lopencv_imgcodecs \
                            -lopencv_imgproc \
                            -lopencv_ml \
                            -lopencv_objdetect \
                            -lopencv_photo \
                            -lopencv_stitching \
                            -lopencv_video \
                            -lopencv_videoio
