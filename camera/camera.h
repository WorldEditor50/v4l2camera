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
#include <set>
#include <map>
#include <iostream>
#include "libyuv.h"
#include "jpegwrap.h"
#include "strings.hpp"

#define CAMERA_PIXELFORMAT_YUYV "YUYV"
#define CAMERA_PIXELFORMAT_JPEG "JPEG"

class Camera
{
public:
    enum Code {
        CODE_OK = 0,
        CODE_DEV_EMPTY = -1,
        CODE_DEV_NOTFOUND = -2,
        CODE_DEV_OPENFAILED = -3
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
    using FnProcessImage = std::function<void(int, int, int, unsigned char*)>;
    static constexpr int mmapBlockCount = 4;
protected:
    /* device */
    int fd;
    std::string devPath;
    /* sample */
    int sampleTimeout;
    int in;
    int out;
    std::atomic<int> isRunning;
    Frame frameBuffer[8];
    Frame outputFrame[8];
    Frame sharedMem[mmapBlockCount];
    FnProcessImage processImage;
    std::thread sampleThread;
    std::thread processThread;
    /* camera property */
    int width;
    int height;
    std::string formatString;
    std::vector<PixelFormat> formatList;
    std::map<std::string, std::vector<std::string> > resolutionMap;
protected:
    static std::string shellExecute(const std::string& command);
    static unsigned short getVendorID(const char* name);
    static unsigned short getProductID(const char* name);
    static int openDevice(const std::string &path);
    void process(Frame &frame, Frame &sharedMem, int size_);
    void onSample();
    void onProcess();
    /* shared memory */
    bool attachSharedMemory();
    void dettachSharedMemory();
    bool checkCapability();
    bool setFormat(int w, int h, const std::string &format);
    int openPath(const std::string &path, const std::string &format, const std::string &res);
    void closeDevice();
public:
    Camera();
    ~Camera();
    static std::vector<Property> enumerate();
    static std::vector<PixelFormat> getPixelFormatList(const std::string &path);
    static std::vector<std::string> getResolutionList(const std::string &path, const std::string &pixelFormat);
    /* register process function */
    void registerProcess(const FnProcessImage &processFunc){processImage = processFunc;}
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

#endif // CAMERA_H
