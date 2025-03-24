#ifndef CAMERA_H
#define CAMERA_H
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
#include <stdarg.h>
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <set>
#include <map>
#include <iostream>
#include "libyuv.h"
#include "jpegwrap.h"
#include "strings.hpp"

#define CAMERA_PIXELFORMAT_YUYV "YUYV"
#define CAMERA_PIXELFORMAT_JPEG "JPEG"
namespace Camera {

enum Code {
    CODE_OK = 0,
    CODE_DEV_EMPTY = -1,
    CODE_DEV_NOTFOUND = -2,
    CODE_DEV_OPENFAILED = -3
};

enum DecodeType {
    Decode_SYNC = 0,
    Decode_ASYNC,
    Decode_PINGPONG
};

enum ParamFlag {
    Param_Auto = 0,
    Param_Manual
};

struct Param {
    int minVal;
    int maxVal;
    int value;
    int defaultVal;
    int step;
    int flag;
};

struct Params {
    Param whiteBalance;
    Param brightness;
    Param contrast;
    Param saturation;
    Param hue;
    Param sharpness;
    Param backlightCompensation;
    Param gamma;
    Param exposure;
    Param gain;
    Param powerLineFrequence;

};

struct DeviceParam {
    int whiteBalanceMode;
    int whiteBalanceTemperature;
    int brightnessMode;
    int brightness;
    int contrast;
    int saturation;
    int hue;
    int sharpness;
    int backlightCompensation;
    int gamma;
    int exposureMode;
    int exposureAbsolute;
    int autoGain;
    int gain;
    int powerLineFrequence;
};
struct Property {
    std::string path;
    unsigned short vendorID;
    unsigned short productID;
};

enum PixelType {
    Pixel_MJPEG = 0,
    Pixel_YUYV
};

struct PixelFormat {
    std::string formatString;
    unsigned int formatInt;
};


class Frame
{
public:
    unsigned char* data;
    unsigned long length;
    unsigned long capacity;
public:
    static unsigned long align(unsigned long s)
    {
        unsigned long size = s;
        if (size&0x3ff) {
            size = ((size >> 10) + 1)<<10;
        }
        return size;
    }
    Frame():data(nullptr), length(0), capacity(0){}
    ~Frame(){}
    void allocate(unsigned long size)
    {
        if (data == nullptr) {
            capacity = align(size);
            length = size;
            data = new unsigned char[capacity];
        } else {
            if (size > capacity) {
                delete [] data;
                capacity = align(size);
                length = size;
                data = new unsigned char[capacity];
            } else {
                length = size;
            }
        }
        return;
    }
    void copy(unsigned char *d, unsigned long s)
    {
        allocate(s);
        memcpy(data, d, length);
        return;
    }
    void clear()
    {
        if (data) {
            delete [] data;
            data = nullptr;
        }
        length = 0;
        capacity = 0;
        return;
    }
};

using FnProcessImage = std::function<void(int, int, int, unsigned char*)>;

class IDecoder
{
protected:
    int width;
    int height;
    std::string formatString;
    FnProcessImage processImage;
public:
    IDecoder(){}
    explicit IDecoder(const FnProcessImage &func):processImage(func){}
    virtual ~IDecoder(){}

    virtual void setFormat(int w, int h, const std::string &format){}

    virtual void sample(unsigned char* data, unsigned long length){}

    virtual void run(){}

    virtual void start(){}

    virtual void stop(){}
};

class Decoder : public IDecoder
{
private:
    int index;
    Frame outputFrame[4];
public:
    Decoder():index(0){}
    explicit Decoder(const FnProcessImage &func)
        :IDecoder(func),index(0){}
    ~Decoder()
    {
        for (int i = 0; i < 4; i++) {
            outputFrame[i].clear();
        }
    }
    virtual void setFormat(int w, int h, const std::string &format) override
    {
        width = w;
        height = h;
        formatString = format;
        unsigned long length = width * height * 4;
        if (formatString == CAMERA_PIXELFORMAT_JPEG) {
            length = Jpeg::align4(width, 3)*height;
        } else if (formatString == CAMERA_PIXELFORMAT_YUYV) {
            length = width * height * 4;
        }
        for (int i = 0; i < 4; i++) {
            outputFrame[i].allocate(length);
        }
        return;
    }

    virtual void sample(unsigned char* data, unsigned long length)
    {
        Frame& frame = outputFrame[index];
        index = (index + 1)%4;
        /* set format */
        if (formatString == CAMERA_PIXELFORMAT_JPEG) {
            Jpeg::decode(frame.data, width, height, data, length, Jpeg::ALIGN_4);
            /* process */
            processImage(height, width, 3, frame.data);
        } else if(formatString == CAMERA_PIXELFORMAT_YUYV) {
            int alignedWidth = (width + 1) & ~1;
            libyuv::YUY2ToARGB(data, alignedWidth * 2,
                    frame.data, width * 4,
                    width, height);
            processImage(height, width, 4, frame.data);
        } else {
            printf("decode failed. format: %s", formatString.c_str());
        }
        return;
    }
};

class AsyncDecoder : public IDecoder
{
public:
    enum State {
        STATE_NONE = 0,
        STATE_PREPENDING,
        STATE_READY,
        STATE_PROCESSING,
        STATE_TERMINATE
    };
private:
    int index;
    int state;
    std::condition_variable condit;
    std::mutex mutex;
    std::thread processThread;
    Frame frameBuffer;
    Frame outputFrame[4];
protected:
    void run()
    {
        printf("enter process function.\n");
        while (1) {
            std::unique_lock<std::mutex> locker(mutex);
            condit.wait_for(locker, std::chrono::milliseconds(1000), [this]()->bool{
                    return state == STATE_TERMINATE || state == STATE_READY;
                    });
            if (state == STATE_TERMINATE) {
                state = STATE_NONE;
                break;
            } else if (state == STATE_PREPENDING) {
                continue;
            }

            state = STATE_PROCESSING;

            Frame& inputFrame = frameBuffer;
            if (inputFrame.data == nullptr) {
                continue;
            }
            Frame& frame = outputFrame[index];
            index = (index + 1)%4;
            /* set format */
            if (formatString == CAMERA_PIXELFORMAT_JPEG) {
                Jpeg::decode(frame.data, width, height, inputFrame.data, inputFrame.length, Jpeg::ALIGN_4);
                /* process */
                processImage(height, width, 3, frame.data);
            } else if(formatString == CAMERA_PIXELFORMAT_YUYV) {
                int alignedWidth = (width + 1) & ~1;
                libyuv::YUY2ToARGB(inputFrame.data, alignedWidth * 2,
                    frame.data, width * 4,
                    width, height);
                processImage(height, width, 4, frame.data);
            } else {
                printf("decode failed. format: %s", formatString.c_str());
            }

            if (state != STATE_TERMINATE) {
                state = STATE_PREPENDING;
                condit.notify_all();
            }

        }
        printf("leave process function.\n");
        return;
    }
public:
    AsyncDecoder():index(0),state(STATE_NONE){}
    explicit AsyncDecoder(const FnProcessImage &func)
        :IDecoder(func),index(0),state(STATE_NONE){}
    ~AsyncDecoder()
    {
        frameBuffer.clear();
        for (int i = 0; i < 4; i++) {
            outputFrame[i].clear();
        }
    }

    virtual void setFormat(int w, int h, const std::string &format) override
    {
        width = w;
        height = h;
        formatString = format;
        unsigned long length = width * height * 4;
        if (formatString == CAMERA_PIXELFORMAT_JPEG) {
            length = Jpeg::align4(width, 3)*height;
        } else if (formatString == CAMERA_PIXELFORMAT_YUYV) {
            length = width * height * 4;
        }
        std::unique_lock<std::mutex> locker(mutex);
        for (int i = 0; i < 4; i++) {
            outputFrame[i].allocate(length);
        }
        return;
    }

    virtual void sample(unsigned char* data, unsigned long length) override
    {
        if (state == STATE_PREPENDING) {
            std::unique_lock<std::mutex> locker(mutex);
            frameBuffer.copy(data, length);
            state = STATE_READY;
            condit.notify_all();
        }
        return;
    }

    virtual void start() override
    {
        if (state != STATE_NONE) {
            return;
        }
        state = STATE_PREPENDING;
        processThread = std::thread(&AsyncDecoder::run, this);
        return;
    }

    virtual void stop() override
    {
        if (state == STATE_NONE) {
            return;
        }
        while (state != STATE_NONE) {
            std::unique_lock<std::mutex> locker(mutex);
            state = STATE_TERMINATE;
            condit.notify_all();
            condit.wait_for(locker, std::chrono::milliseconds(500), [=]()->bool{
                    return state == STATE_NONE;
            });
        }
        processThread.join();
        return;
    }

};

class PingPongDecoder : public IDecoder
{
public:
    constexpr static int max_buffer_len = 8;
private:
    int in;
    int out;
    std::atomic<bool> isRunning;
    std::thread processThread;
    Frame frameBuffer[8];
    Frame outputFrame[8];
protected:
    virtual void run() override
    {
        printf("enter process function.\n");
        while (isRunning.load()) {
            int index = out;
            Frame& inputFrame = frameBuffer[index];
            if (inputFrame.data == nullptr) {
                continue;
            }
            Frame& frame = outputFrame[index];
            /* set format */
            if (formatString == CAMERA_PIXELFORMAT_JPEG) {
                Jpeg::decode(frame.data, width, height, inputFrame.data, inputFrame.length, Jpeg::ALIGN_4);
                /* process */
                processImage(height, width, 3, frame.data);
            } else if(formatString == CAMERA_PIXELFORMAT_YUYV) {
                int alignedWidth = (width + 1) & ~1;
                libyuv::YUY2ToARGB(inputFrame.data, alignedWidth * 2,
                    frame.data, width * 4,
                    width, height);
                processImage(height, width, 4, frame.data);
            } else {
                printf("decode failed. format: %s", formatString.c_str());
            }

        }
        printf("leave process function.\n");
        return;
    }
public:
    PingPongDecoder():in(0),out(0),isRunning(false){}
    explicit PingPongDecoder(const FnProcessImage &func)
        :IDecoder(func),in(0),out(0),isRunning(false){}
    ~PingPongDecoder()
    {
        for (int i = 0; i < 8; i++) {
            frameBuffer[i].clear();
            outputFrame[i].clear();
        }
    }

    virtual void setFormat(int w, int h, const std::string &format) override
    {
        width = w;
        height = h;
        formatString = format;
        unsigned long length = width * height * 4;
        if (formatString == CAMERA_PIXELFORMAT_JPEG) {
            length = Jpeg::align4(width, 3)*height;
        } else if (formatString == CAMERA_PIXELFORMAT_YUYV) {
            length = width * height * 4;
        }
        for (int i = 0; i < 8; i++) {
            outputFrame[i].allocate(length);
        }
        return;
    }

    virtual void sample(unsigned char* data, unsigned long length) override
    {
        frameBuffer[in].copy(data, length);
        out = in;
        in = (in + 1)%8;
        return;
    }

    virtual void start() override
    {
        if (isRunning.load()) {
            return;
        }
        isRunning.store(true);
        processThread = std::thread(&PingPongDecoder::run, this);
        return;
    }

    virtual void stop() override
    {
        if (isRunning.load()) {
            isRunning.store(false);
            processThread.join();
        }
        return;
    }
};

class Device
{
public:
    static constexpr int mmapBlockCount = 4;
protected:
    /* device */
    int fd;
    std::string devPath;
    IDecoder *decoder;
    /* sample */
    int sampleTimeout;
    std::atomic<int> isRunning;
    Frame sharedMem[mmapBlockCount];
    std::thread sampleThread;
    /* camera property */
    std::vector<PixelFormat> formatList;
    std::map<std::string, std::vector<std::string> > resolutionMap;
protected:
    static std::string shellExecute(const std::string& command);
    static unsigned short getVendorID(const char* name);
    static unsigned short getProductID(const char* name);
    static int openDevice(const std::string &path);
    void onSample();
    /* shared memory */
    bool attachSharedMemory();
    void dettachSharedMemory();
    bool checkCapability();
    bool setFormat(int w, int h, const std::string &format);
    int openPath(const std::string &path, const std::string &format, const std::string &res);
    void closeDevice();
public:
    explicit Device(int decodeType, const FnProcessImage &func);
    ~Device();
    static std::vector<Property> enumerate();
    static std::vector<PixelFormat> getPixelFormatList(const std::string &path);
    static std::vector<std::string> getResolutionList(const std::string &path, const std::string &pixelFormat);
    /* start - stop */
    int start(const std::string &path, const std::string &format, const std::string &res);
    int start(unsigned short vid, unsigned short pid, const std::string &pixelFormat, int resIndex=0);
    void stop();
    bool startSample();
    bool stopSample();
    void clear();
    void restart(const std::string &format, const std::string &res);
    /* parameter */
    void setParam(unsigned int controlID, int value);
    int getParamRange(unsigned int controlID, int modeID, Param &param);
    int getParam(unsigned int controlID);
    /* white balance */
    void setWhiteBalanceMode(int value = V4L2_WHITE_BALANCE_MANUAL);
    int getWhiteBalanceMode();
    void setWhiteBalanceTemperature(int value);
    int getWhiteBalanceTemperature();
    /* brightness */
    void setBrightnessMode(int value);
    int getBrightnessMode();
    void setBrightness(int value);
    int getBrightness();
    /* contrast */
    void setContrast(int value);
    int getContrast();
    /* saturation */
    void setSaturation(int value);
    int getSaturation();
    /* hue */
    void setHue(int value);
    int getHue();
    /* sharpness */
    void setSharpness(int value);
    int getSharpness();
    /* backlight compensation */
    void setBacklightCompensation(int value);
    int getBacklightCompensation();
    /* gamma */
    void setGamma(int value);
    int getGamma();
    /* exposure */
    void setExposureMode(int value = V4L2_EXPOSURE_MANUAL);
    int getExposureMode();
    void setExposure(int value);
    int getExposure();
    void setExposureAbsolute(int value);
    int getExposureAbsolute();
    /* gain */
    void setAutoGain(int value);
    int getAutoGain();
    void setGain(int value);
    int getGain();
    /* frequency */
    void setPowerLineFrequence(int value);
    int getFrequency();
    /* default paramter */
    void setDefaultParam();
    void setParam(const DeviceParam &param);

};

}
#endif // CAMERA_H
