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
#include <functional>
#include <iostream>
#include "v4l2param.h"
#include "libyuv.h"
#include "libyuv/convert_argb.h"

template <typename T>
class V4l2Allocator
{
private:
    static QMap<std::size_t, QVector<T*> > mem;
    static QMutex mutex;
private:
    V4l2Allocator() = default;
    ~V4l2Allocator()
    {
        clear();
    }
public:

    static T* get(std::size_t size_)
    {
        if (size_ == 0) {
            return nullptr;
        }
        QMutexLocker locker(&mutex);
        T* ptr = nullptr;
        if (mem.find(size_) == mem.end()) {
            ptr = new T[size_];
        } else {
            ptr = mem[size_].back();
            mem[size_].pop_back();
        }
        return ptr;
    }
    static void recycle(std::size_t size_, T* &ptr)
    {
        if (size_ == 0 || ptr == nullptr) {
            return;
        }
        QMutexLocker locker(&mutex);
        mem[size_].push_back(ptr);
        ptr = nullptr;
        return;
    }
    static void clear()
    {
        QMutexLocker locker(&mutex);
        for (auto& block : mem) {
            for (int i = 0; i < block.size(); i++) {
                T* ptr = block.at(i);
                delete [] ptr;
            }
            block.clear();
        }
        mem.clear();
        return;
    }
};
template <typename T>
QMap<unsigned long, QVector<T*> > V4l2Allocator<T>::mem;
template <typename T>
QMutex V4l2Allocator<T>::mutex;

class V4l2Camera : public QObject
{
    Q_OBJECT
public:
    struct SharedMemory {
        void *start;
        int length;
    };
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
    using ProcessFunc = std::function<QImage(int, int, unsigned char*)>;
protected:
    int fd;
    int mmapBlockCount;
    int sampleTimeout;
    QString devPath;
    QVector<Format> formatList;
    QString formatDesc;
    QAtomicInt isRunning;
    QVector<SharedMemory> sharedMemoryBlock;
    QFuture<void> future;
    V4l2Param param;
    ProcessFunc processImage;
signals:
    void send(const QImage &img);
public:
    explicit V4l2Camera(QObject *parent = nullptr):QObject(parent),
        fd(-1), mmapBlockCount(4), sampleTimeout(3),
        formatDesc(FORMAT_JPEG), isRunning(0),
        processImage([](int width, int height, unsigned char* data)->QImage {
            return QImage(data, width, height, QImage::Format_ARGB32);
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
    static QString shellExecute(const QString& command);
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
