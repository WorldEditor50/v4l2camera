#ifndef CAMERADEVICE_H
#define CAMERADEVICE_H
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
#include "libyuv.h"
#include "libyuv/convert_argb.h"
#include "logger.hpp"
#include "v4l2param.h"
#include "reallocator.hpp"
#include "imageprocess.h"

struct FrameData
{
    unsigned int width;
    unsigned int height;
};

class PixelFormat
{
public:
    QString formatString;
    unsigned int formatInt;
    QVector<FrameData> resolutionList;
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

struct CameraInfo
{
    QString devPath;
    QString cardName;
    QVector<PixelFormat> formatList;
};


struct SharedMemBlock
{
    void *start;
    int length;
};

class V4l2Device : public QObject
{
    Q_OBJECT
public:
    enum State {
        SAMPLE_RUNNING = 0,
        SAMPLE_PAUSE,
        SAMPLE_TERMINATE
    };
    using ImageDataPool = Reallocator<unsigned char>;
public:
    int fd;
    int mmapBlockCount;
    int sampleTimeout;
    int compressScale;
    QString path;
    QString formatString;
    QMutex mutex;
    QWaitCondition condit;
    State state;
    QVector<SharedMemBlock> memoryVec;
    QFuture<void> future;
    V4l2Param param;
signals:
    void send(const QImage &img);
public:
    V4l2Device():
        fd(-1), mmapBlockCount(8), sampleTimeout(5), compressScale(1), state(SAMPLE_PAUSE){}
    V4l2Device(const QString &path_, int fd_):
        fd(fd_), mmapBlockCount(8), sampleTimeout(5), compressScale(1),
        path(path_), formatString(FORMAT_JPEG), state(SAMPLE_PAUSE){}
    ~V4l2Device();
    inline bool isValidJpeg(const unsigned char* data, unsigned long len)
    {
        return (data != nullptr && len > 2) &&
                (data[0] == 0xff && data[1] == 0xd8 &&
                data[len - 2] == 0xff && data[len - 1] == 0xd9);
    }
    void process(int index, int size_);
    void sampling();
    /* open */
    static int openDevice(const QString &path);
    /* get - set */
    bool isSupportParam(unsigned int controlID);
    int getParam(unsigned int controlID);
    void setParam(unsigned int controlID, int value = 0);
    bool checkCapability();
    bool setFormat(int w, int h, const QString &format);
    void setAutoParam();
    bool setDefaultParam();
    /* save configuration */
    QMap<QString, int> updateParams();
    void saveParams(const QString &paramXml);
    void loadParams(const QString &paramXml);
    /* shared memory */
    bool attachSharedMemory();
    void dettachSharedMemory();
    /* sample */
    bool startSample();
    bool stopSample();
    void pauseSample();
    void wakeUpSample();
    void terminateSample();
    void clear();
    /* start */
    void restart(const QString &format, int width, int height);
};


#endif // CAMERADEVICE_H
