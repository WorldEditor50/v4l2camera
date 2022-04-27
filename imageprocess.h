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
    using Func = std::function<QImage(int, int, unsigned char*)>;
public:
    static QImage mat2QImage(const cv::Mat &src);
    inline static QImage linear(int width, int height, unsigned char *data)
    {
        return QImage(data, width, height, QImage::Format_ARGB32);
    }
    static QImage canny(int width, int height, unsigned char* data);
    static QImage laplace(int width, int height, unsigned char* data);
    static QImage yolov5(int width, int height, unsigned char* data);
    inline Func& invoke(const QString &funcName){return methods[funcName];}
    inline static Imageprocess& instance()
    {
        static Imageprocess processor;
        return processor;
    }
private:
    Imageprocess()
    {
        methods.insert("none", &Imageprocess::linear);
        methods.insert("canny", &Imageprocess::canny);
        methods.insert("laplace", &Imageprocess::laplace);
        methods.insert("yolov5", &Imageprocess::yolov5);
    }
private:
    QMap<QString, Func> methods;
};

#endif // IMAGEPROCESS_H
