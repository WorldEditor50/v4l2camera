#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <QVector>
#include <QMap>
#include <QString>
#include <QAtomicInt>
#include <QWaitCondition>
#include <QMutex>
#include <QImage>
#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <QProcess>
#include <functional>
#include <iostream>
#include "v4l2param.h"
#include "libyuv.h"
#include "libyuv/convert_argb.h"

class Image
{
public:
    int width;
    int height;
    int channel;
    unsigned char *ptr;
    std::size_t size_;
public:
    Image():width(0),height(0),channel(3),ptr(nullptr),size_(0){}
    Image(int w, int h, int c):width(w),height(h),channel(c)
    {
        size_ = w * h * c;
        ptr = new unsigned char[size_];
    }
    Image(const Image &img):width(img.width),height(img.height),channel(img.channel)
    {
        size_ = width * height * channel;
        ptr = new unsigned char[size_];
        memcpy(ptr, img.ptr, size_);
    }
    Image &operator=(const Image &img)
    {
        if (this == &img) {
            return *this;
        }
        width = img.width;
        height = img.height;
        channel = img.channel;
        size_ = width * height * channel;
        ptr = new unsigned char[size_];
        memcpy(ptr, img.ptr, size_);
        return *this;
    }
    Image(Image &&img)
        :width(img.width),height(img.height),channel(img.channel),
          ptr(img.ptr),size_(img.size_)
    {
        img.width = 0;
        img.height = 0;
        img.channel = 0;
        img.ptr = nullptr;
        img.size_ = 0;
    }
    Image &operator=(Image &&img)
    {
        if (this == &img) {
            return *this;
        }
        width = img.width;
        height = img.height;
        channel = img.channel;
        size_ =img.size_;
        ptr = img.ptr;
        img.width = 0;
        img.height = 0;
        img.channel = 0;
        img.ptr = nullptr;
        img.size_ = 0;
        return *this;
    }
    ~Image()
    {
        clear();
    }
    void clear()
    {
        if (ptr != nullptr) {
            delete [] ptr;
            ptr = nullptr;
            width = 0;
            height = 0;
            channel = 0;
            size_ = 0;
        }
        return;
    }
};

class V4l2Camera : public QObject
{
    Q_OBJECT
public:
    struct Resolution {
        unsigned int width;
        unsigned int height;
    };
    class Format
    {
    public:
        QString description;
        unsigned int value;
        QVector<Resolution> resolutionList;
    public:
        bool isExist(unsigned int w, unsigned int h)
        {
            bool ret = false;
            for (auto& x: resolutionList) {
                if (x.width == w && x.height == h) {
                    ret = true;
                    break;
                }
            }
            return ret;
        }
    };
    using ProcessFunc = std::function<void(unsigned char*, int, int)>;
    static int maxDeviceNum;
protected:
    int fd;
    static constexpr int mmapBlockCount = 4;
    int sampleTimeout;
    QString devPath;
    QVector<Format> formatList;
    QString formatDesc;
    QAtomicInt isRunning;
    Image sharedMemory[mmapBlockCount];
    Image images[mmapBlockCount];
    QFuture<void> future;
    V4l2Param param;
    ProcessFunc processImage;
public:
    explicit V4l2Camera(QObject *parent = nullptr):QObject(parent),
        fd(-1), sampleTimeout(3),
        formatDesc(FORMAT_JPEG), isRunning(0),
        processImage([](unsigned char* data, int width, int height) {
            QImage(data, width, height, QImage::Format_ARGB32);
        }) {}
    ~V4l2Camera();
    static QStringList findDevice(const QString &key, const std::function<bool(const QString &, const QString &)> &func);
    static QStringList findAllDevice();
    static QStringList findDeviceByVidPid(const QString &vidpid);
    static QStringList findDeviceByVid(const QString &vid);
    /* register process function */
    void setProcessFunc(const ProcessFunc &processFunc){processImage = processFunc;}
    /* get */
    QStringList getFormatList();
    QStringList getResolutionList();
    int getWhiteBalanceMode();
    int getWhiteBalanceTemperature();
    int getBrightnessMode();
    int getBrightness();
    int getContrast();
    int getSaturation();
    int getHue();
    int getSharpness();
    int getBacklightCompensation();
    int getGamma();
    int getExposureMode();
    int getExposure();
    int getExposureAbsolute();
    int getGainMode();
    int getGain();
    int getFrequency();
public slots:
    void openPath(const QString &path);
    /* white balance */
    void setWhiteBalanceMode(int value = V4L2_WHITE_BALANCE_MANUAL);
    void setWhiteBalanceTemperature(int value);
    /* brightness */
    void setBrightnessMode(int value);
    void setBrightness(int value);
    /* contrast */
    void setContrast(int value);
    /* saturation */
    void setSaturation(int value);
    /* hue */
    void setHue(int value);
    /* sharpness */
    void setSharpness(int value);
    /* backlight compensation */
    void setBacklightCompensation(int value);
    /* gamma */
    void setGamma(int value);
    /* exposure */
    void setExposureMode(int value = V4L2_EXPOSURE_MANUAL);
    void setExposure(int value);
    void setExposureAbsolute(int value);
    /* gain */
    void setGainMode(int value);
    void setGain(int value);
    /* frequency */
    void setFrequency(int value);
    /* start - stop */
    void start(const QString &format, const QString &res);
    void restart(const QString &format, const QString &res);
    void clear();
    /* save configuration */
    void setAutoParam();
    void setDefaultParam();
    void saveParams(const QString &paramXml);
    void loadParams(const QString &paramXml);
protected:
    /* device */
    static QString executeCmd(const QString& cmd);
    static QString getVidPid(const QString &name);
    static int openDevice(const QString &path);
    void closeDevice();
    bool checkCapability();
    bool setFormat(int w, int h, const QString &format);
    /* parameter */
    bool isSupportParam(unsigned int controlID);
    int getParam(unsigned int controlID);
    void setParam(unsigned int controlID, int value = 0);
    /* shared memory */
    bool attachSharedMemory();
    void dettachSharedMemory();
    inline bool isValidJpeg(const unsigned char* data, unsigned long len)
    {
        return (data != nullptr && len > 2) &&
                (data[0] == 0xff && data[1] == 0xd8 &&
                data[len - 2] == 0xff && data[len - 1] == 0xd9);
    }
    /* sample */
    void process(int index, int size_);
    void doSample();
    bool startSample();
    bool stopSample();
};


#endif // V4L2CAMERA_H
