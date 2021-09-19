#include "imageprocess.h"

int Imageprocess::Type = CV_8UC4;

QImage Imageprocess::fromMat(const cv::Mat &src)
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
        for(int i = 0; i < src.rows ; i++){
            uchar* rowdata = img.scanLine( i );
            memcpy(rowdata, data , src.cols);
            data += src.cols;
        }
        break;
    }
    return img;
}

QImage Imageprocess::invoke(int width, int height, unsigned char *data)
{
    if (data == nullptr) {
        return QImage();
    }
    cv::Mat src(height, width, Imageprocess::Type, data);
    cv::cvtColor(src, src, cv::COLOR_RGBA2GRAY);
    cv::blur(src, src, cv::Size(3, 3));
    cv::Canny(src, src, 60, 120);
    return fromMat(src);
}

QImage Imageprocess::laplace(int width, int height, unsigned char *data)
{
    /* laplace  */
    cv::Mat src(height, width, Imageprocess::Type, data);
    cv::cvtColor(src, src, cv::COLOR_RGBA2GRAY);
    cv::blur(src, src, cv::Size(3, 3));
    cv::Canny(src, src, 60, 150);
    cv::Mat gaussBlurImg;
    cv::GaussianBlur(src, gaussBlurImg, cv::Size(3, 3), 0);
    cv::Mat gray;
    cv::cvtColor(gaussBlurImg, gray, cv::COLOR_RGBA2GRAY);
    cv::Mat filterImg;
    cv::Laplacian(gray, filterImg, CV_16S, 3);
    cv::Mat dst;
    cv::convertScaleAbs(filterImg, dst);
    return fromMat(dst);
}


