#ifndef IMAGEPROCESS_H
#define IMAGEPROCESS_H
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QImage>
#include <QMap>
#include "yolov5.h"

class Imageprocess
{
public:
    static void canny(int height, int width, unsigned char* data);
    static void laplace(int height, int width, unsigned char* data);
    static void yolov5(int height, int width, unsigned char* data);
};

#endif // IMAGEPROCESS_H
