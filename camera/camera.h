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
    struct Frame {
        unsigned char* data;
        std::size_t length;
    };
    struct Property {
        std::string path;
        unsigned short vendorID;
        unsigned short productID;
    };
    struct PixelFormat {
        std::string formatString;
        unsigned int formatInt;
    };
    using FnProcessImage = std::function<void(int, int, int, unsigned char*)>;
    static constexpr int mmapBlockCount = 8;
protected:
    /* device */
    int fd;
    std::string devPath;
    /* sample */
    int sampleTimeout;
    std::atomic<int> isRunning;
    Frame frameBuffer[mmapBlockCount];
    Frame sharedMemFrame[mmapBlockCount];
    FnProcessImage processImage;
    std::thread sampleThread;
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
    void run();
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

struct Strings {

    static std::string format(int totalsize, const char *formatText, ...)
    {
        std::string text(totalsize, 0);
        va_list ap;
        va_start(ap, formatText);
        ::vsprintf((char*)text.c_str(), formatText, ap);
        va_end(ap);
        return text;
    }

    static std::string lower(const std::string &s_)
    {
        std::string s(s_);
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] >= 97 && s[i] <= 122) {
                s[i] -= 32;
            }
        }
        return s;
    }

    static std::string upper(const std::string &s_)
    {
        std::string s(s_);
        for (std::size_t i = 0; i < s.size(); i++) {
            if (s[i] >= 65 && s[i] <= 90) {
                s[i] += 32;
            }
        }
        return s;
    }

    static std::vector<std::string> split(const std::string& src, const std::string& delim)
    {
        std::vector<std::string> elems;
        std::size_t pos = 0;
        std::size_t len = src.length();
        std::size_t delim_len = delim.length();
        if (delim_len == 0) {
            return elems;
        }
        while (pos < len) {
            int find_pos = src.find(delim, pos);
            if (find_pos < 0) {
                elems.push_back(src.substr(pos, len - pos));
                break;
            }
            elems.push_back(src.substr(pos, find_pos - pos));
            pos = find_pos + delim_len;
        }
        return elems;
    }

    static int stringToInt(const std::string &text)
    {
        return std::atoi(text.c_str());
    }

    static char hexCharToInt4(char c)
    {
        char x = 0;
        if (c >= 'A' && c <= 'Z') {
            x = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'z') {
            x = c - 'a' + 10;
        } else if (c >= '0' && c <= '9') {
            x = c - '0';
        }
        return x;
    }

    static unsigned char hexStringToInt8(const char* hex)
    {
        unsigned char x0 = hexCharToInt4(hex[1]);
        unsigned char x1 = hexCharToInt4(hex[0]);
        return (x1<<4) + x0;
    }

    static unsigned short hexStringToInt16(const std::string &hex)
    {
        unsigned char x0 = hexCharToInt4(hex[3]);
        unsigned char x1 = hexCharToInt4(hex[2]);
        unsigned char x2 = hexCharToInt4(hex[1]);
        unsigned char x3 = hexCharToInt4(hex[0]);
        return (x3<<12) + (x2<<8) + (x1<<4) + x0;
    }


};


#endif // CAMERA_H
