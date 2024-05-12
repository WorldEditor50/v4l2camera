#include "imageprocess.h"
#include <QDebug>

void Imageprocess::canny(int height, int width, unsigned char *data)
{
    cv::Mat img(height, width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_RGB2GRAY);
    cv::blur(gray, gray, cv::Size(3, 3));
    cv::Canny(gray, gray, 60, 120);
    cv::cvtColor(gray, img, cv::COLOR_GRAY2RGB);
    return;
}

void Imageprocess::laplace(int height, int width, unsigned char *data)
{
    cv::Mat img(height, width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_RGB2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);
    cv::Mat filterImg;
    cv::Laplacian(gray, filterImg, CV_16S, 3);
    cv::Mat scaleImg;
    cv::convertScaleAbs(filterImg, scaleImg);
    cv::cvtColor(scaleImg, img, cv::COLOR_GRAY2RGB);
    return ;
}

void Imageprocess::yolov5(int height, int width, unsigned char *data)
{
    cv::Mat img(height, width, CV_8UC3, data);
    {
        ncnn::MutexLockGuard guard(Yolov5::instance().lock);
        std::vector<Yolov5::Object> objects;
        Yolov5::instance().detect(img, objects);
        Yolov5::instance().draw(img, objects);
    }
    return;
}


