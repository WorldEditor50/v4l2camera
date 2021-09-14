#ifndef IMAGEPROCESS_H
#define IMAGEPROCESS_H
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QImage>

class Imageprocess
{
public:
    static int Type;
    static QImage fromMat(const cv::Mat &src);
    static QImage invoke(int width, int height, unsigned char* data);
    static QImage laplace(int width, int height, unsigned char* data);
};

#endif // IMAGEPROCESS_H
