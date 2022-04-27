#include "imageprocess.h"
#include <QDebug>

QImage Imageprocess::mat2QImage(const cv::Mat &src)
{
    QImage img;
    int channel = src.channels();
    switch (channel) {
    case 3:
        img = QImage(src.data, src.cols, src.rows, QImage::Format_RGB888);
        break;
    case 4:
        img = QImage(src.data, src.cols, src.rows, QImage::Format_ARGB32);
        break;
    default:
        img = QImage(src.cols, src.rows, QImage::Format_Indexed8);
        uchar *data = src.data;
        for (int i = 0; i < src.rows ; i++){
            uchar* rowdata = img.scanLine(i);
            memcpy(rowdata, data , src.cols);
            data += src.cols;
        }
        break;
    }
    return img;
}

QImage Imageprocess::canny(int width, int height, unsigned char *data)
{
    if (data == nullptr) {
        return QImage();
    }
    cv::Mat src(height, width, CV_8UC4, data);
    cv::cvtColor(src, src, cv::COLOR_RGBA2GRAY);
    cv::blur(src, src, cv::Size(3, 3));
    cv::Canny(src, src, 60, 120);
    return mat2QImage(src);
}

QImage Imageprocess::laplace(int width, int height, unsigned char *data)
{
    /* laplace  */
    cv::Mat src(height, width, CV_8UC4, data);
    cv::Mat gaussBlurImg;
    cv::GaussianBlur(src, gaussBlurImg, cv::Size(3, 3), 0);
    cv::Mat gray;
    cv::cvtColor(gaussBlurImg, gray, cv::COLOR_RGBA2GRAY);
    cv::Mat filterImg;
    cv::Laplacian(gray, filterImg, CV_16S, 3);
    cv::Mat dst;
    cv::convertScaleAbs(filterImg, dst);
    return mat2QImage(dst);
}

QImage Imageprocess::yolov5(int width, int height, unsigned char *data)
{
    cv::Mat src(height, width, CV_8UC4, data);
    std::vector<Yolov5::Object> objects;
    Yolov5::instance().detect(src, objects);
    Yolov5::instance().draw(src, objects);
    return mat2QImage(src);
}


