#ifndef IMAGEPROCESS_H
#define IMAGEPROCESS_H
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QImage>
#include "yolov5.h"

class Imageprocess
{
public:
    static int steps;
    static QImage fromMat(const cv::Mat &src);
    static QImage invoke(int width, int height, unsigned char* data);
    static QImage laplace(int width, int height, unsigned char* data);
    static QImage yolov5(int width, int height, unsigned char* data);
};

#endif // IMAGEPROCESS_H
