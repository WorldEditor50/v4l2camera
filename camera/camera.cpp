#include "camera.h"
#include <unistd.h>
#include <algorithm>

Camera::Camera()
    :fd(-1),sampleTimeout(5),isRunning(0),in(0),out(0),
    formatString(CAMERA_PIXELFORMAT_JPEG)
{

}

Camera::~Camera()
{
    for (int i = 0; i < 2; i++) {
        frameBuffer[i].clear();
        outputFrame[i].clear();
    }
}

void Camera::onSample()
{
    printf("enter sampling function.\n");
    while (isRunning.load()) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        /*Timeout*/
        struct timeval tv;
        tv.tv_sec = sampleTimeout;
        tv.tv_usec = 0;
        int ret = select(fd + 1, &fds, NULL, NULL, &tv);
        if (ret == -1) {
            if (EINTR == errno) {
                continue;
            }
            perror("Fail to select");
            continue;
        }
        if (ret == 0) {
            fprintf(stderr,"select Timeout\n");
            continue;
        }
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        // put cache from queue
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            perror("0 Fail to ioctl 'VIDIOC_DQBUF'");
            continue;
        }
        /* copy */
        frameBuffer[in].copy(sharedMem[buf.index].data, buf.bytesused);
        out = in;
        in = (in + 1)%2;
        /* dequeue */
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("0 Fail to ioctl 'VIDIOC_QBUF'");
        }
    }
    printf("leave sampling function.\n");
    return;
}

void Camera::onProcess()
{
    printf("enter process function.\n");
    while (isRunning.load()) {
        Frame& inputFrame = frameBuffer[out];
        if (inputFrame.data == nullptr) {
            continue;
        }
        unsigned long len = width * height * 4;
        if (formatString == CAMERA_PIXELFORMAT_JPEG) {
            len = Jpeg::align4(width, 3)*height;
        } else if (formatString == CAMERA_PIXELFORMAT_YUYV) {
            len = width * height * 4;
        }
        /* allocate memory for image */
        outputFrame[out].allocate(len);
        Frame& frame = outputFrame[out];
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

int Camera::openDevice(const std::string &path)
{
    int fd = open(path.c_str(), O_RDWR, 0);
    if (fd < 0) {
        perror("at Camera::openDevice, fail to open device, error");
        return -1;
    };
    /* input */
    struct v4l2_input input;
    input.index = 0;
    if (ioctl(fd, VIDIOC_S_INPUT, &input) == -1) {
        perror("Failed to ioctl VIDIOC_S_INPUT");
        close(fd);
        return -2;
    }
    /* frame */
    v4l2_streamparm streamParam;
    memset(&streamParam, 0, sizeof(struct v4l2_streamparm));
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamParam.parm.capture.timeperframe.numerator = 1;
    streamParam.parm.capture.timeperframe.denominator = 2;
    if (ioctl(fd, VIDIOC_S_PARM, &streamParam) == -1) {
        perror("failed to set frame");
    }
    return fd;
}

bool Camera::checkCapability()
{
    /* check video decive driver capability */
    struct v4l2_capability 	cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        fprintf(stderr, "fail to ioctl VIDEO_QUERYCAP \n");
        close(fd);
        fd = -1;
        return false;
    }

    if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        fprintf(stderr, "The Current device is not a video capture device \n");
        close(fd);
        fd = -1;
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("The Current device does not support streaming i/o\n");
        close(fd);
        fd = -1;
        return false;
    }
    return true;
}

bool Camera::setFormat(int w, int h, const std::string &format)
{
    if (format.empty()) {
        perror("farmat is empty");
        return false;
    }
    width = w;
    height = h;
    /* set format */
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = w;
    fmt.fmt.pix.height = h;
    if (format == CAMERA_PIXELFORMAT_JPEG) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    } else if (format == CAMERA_PIXELFORMAT_YUYV) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    }
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT set err");
        return false;
    }
    formatString = format;
    return true;
}

bool Camera::attachSharedMemory()
{
    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.count = mmapBlockCount;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbufs) == -1) {
        perror("Fail to ioctl 'VIDIOC_REQBUFS'");
        close(fd);
        fd = -1;
        return false;
    }
    /* map kernel cache to user process */
    for (std::size_t i = 0; i < mmapBlockCount; i++) {
        //stand for a frame
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        /*check the information of the kernel cache requested*/
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("Fail to ioctl : VIDIOC_QUERYBUF");
            close(fd);
            fd = -1;
            return false;
        }
        sharedMem[i].length = buf.length;
        sharedMem[i].data = (unsigned char*)mmap(NULL, buf.length,
                                                 PROT_READ | PROT_WRITE, MAP_SHARED,
                                                 fd, buf.m.offset);
        if (sharedMem[i].data == MAP_FAILED) {
            perror("Fail to mmap");
            close(fd);
            fd = -1;
            return false;
        }
    }
    /* put the kernel cache to a queue */
    for (std::size_t i = 0; i < mmapBlockCount; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Fail to ioctl 'VIDIOC_QBUF'");
            closeDevice();
            return false;
        }
    }
    return true;
}

void Camera::dettachSharedMemory()
{
    for (std::size_t i = 0; i < mmapBlockCount; i++) {
        if (munmap(sharedMem[i].data, sharedMem[i].length) == -1) {
            perror("Fail to munmap");
        }
    }
    return;
}

bool Camera::startSample()
{
    if (isRunning.load()) {
        return true;
    }
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("VIDIOC_STREAMON");
        closeDevice();
        return false;
    }
    /* start thread */
    isRunning.store(1);
    sampleThread = std::thread(&Camera::onSample, this);
    processThread = std::thread(&Camera::onProcess, this);
    return true;
}

bool Camera::stopSample()
{
    if (isRunning.load()) {
        isRunning.store(0);
        v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
            perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
        }
        sampleThread.join();
        processThread.join();
    }
    return true;
}

void Camera::closeDevice()
{
    /* dettach shared memory */
    dettachSharedMemory();
    /* close device */
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    return;
}

std::string Camera::shellExecute(const std::string& command)
{
    std::string result = "";
    FILE *fpRead = popen(command.c_str(), "r");
    char buf[1024];
    memset(buf,'\0',sizeof(buf));
    while (fgets(buf, 1024-1, fpRead)!=NULL) {
        result = buf;
    }
    if (fpRead != NULL) {
        pclose(fpRead);
    }
    auto it = result.find('\n');
    result.erase(it);
    return result;
}

unsigned short Camera::getVendorID(const char *name)
{
    std::string cmd = Strings::format(1024, "cat /sys/class/video4linux/%s/device/modalias", name);
    /* usb:v2B16p6689d0100dcEFdsc02dp01ic0Eisc01ip00in00 */
    std::string result = shellExecute(cmd);
    int i = result.find('v');
    std::string vid = result.substr(i + 1, 4);
    return Strings::hexStringToInt16(vid);
}
unsigned short Camera::getProductID(const char *name)
{
    std::string cmd = Strings::format(1024, "cat /sys/class/video4linux/%s/device/modalias", name);
    /* usb:v2B16p6689d0100dcEFdsc02dp01ic0Eisc01ip00in00 */
    std::string result = shellExecute(cmd);
    int i = result.find('p');
    std::string pid = result.substr(i + 1, 4);
    return Strings::hexStringToInt16(pid);
}

std::vector<Camera::Property> Camera::enumerate()
{
    std::vector<Camera::Property> devPathList;
    DIR *dir;
    if ((dir = opendir("/dev")) == nullptr) {
        printf("failed to open /dev/\n");
        return devPathList;
    }
    struct dirent *ptr = nullptr;
    while ((ptr=readdir(dir)) != nullptr) {
        if (ptr->d_type != DT_CHR) {
            continue;
        }
        if (std::string(ptr->d_name).find("video") == std::string::npos) {
            continue;
        }
        Camera::Property property;
        property.vendorID = Camera::getVendorID((char*)ptr->d_name);
        property.productID = Camera::getProductID((char*)ptr->d_name);

        std::string devPath = Strings::format(32, "/dev/%s", (char*)ptr->d_name);
        int index = devPath.find('\0');
        property.path = devPath.substr(0, index);
        int fd = open(property.path.c_str(), O_RDWR | O_NONBLOCK, 0);
        /* check capability */
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc,0, sizeof(fmtdesc));
        fmtdesc.index = 0;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)<=-1) {
            continue;
        }
        devPathList.push_back(property);
        close(fd);
    }
    return devPathList;
}

std::vector<Camera::PixelFormat> Camera::getPixelFormatList(const std::string &path)
{
    std::vector<PixelFormat> formatList;
    int fd = open(path.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        printf("fail to open device. fd = %d\n", fd);
        return formatList;
    }

    struct v4l2_capability 	cap;
    memset(&cap, 0, sizeof(cap));
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap)<0) {
        perror("VIDIOC_QUERYCAP fail");
        close(fd);
        return formatList;
    }
    if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        perror("not V4L2_BUF_TYPE_VIDEO_CAPTURE");
        close(fd);
        return formatList;
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        perror("not V4L2_CAP_STREAMING");
        close(fd);
        return formatList;
    }
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc,0, sizeof(fmtdesc));
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (1) {
        int ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);
        if (ret == -1) {
            printf("%s\n",strerror(errno));
            break;
        }
        std::string description = std::string((char*)fmtdesc.description);
        PixelFormat pixelFormat;
        if (description.find(CAMERA_PIXELFORMAT_JPEG) != std::string::npos) {
            pixelFormat.formatString = CAMERA_PIXELFORMAT_JPEG;
        } else if (description.find(CAMERA_PIXELFORMAT_YUYV) != std::string::npos) {
            pixelFormat.formatString = CAMERA_PIXELFORMAT_YUYV;
        } else {
            pixelFormat.formatString = description;
        }
        pixelFormat.formatInt = fmtdesc.pixelformat;
        formatList.push_back(pixelFormat);
        fmtdesc.index++;
    }
    close(fd);
    return formatList;
}

std::vector<std::string> Camera::getResolutionList(const std::string &path, const std::string &pixelFormat)
{
    std::vector<std::string> resList;
    /* get pixel format list */
    std::vector<PixelFormat> pixelFormatList = Camera::getPixelFormatList(path);
    if (pixelFormatList.empty()) {
        return resList;
    }

    bool hasPixelFormat = false;
    unsigned int pixelFormatInt = 0;
    for (std::size_t i = 0; i < pixelFormatList.size(); i++) {
        if (pixelFormatList[i].formatString == pixelFormat) {
            hasPixelFormat = true;
            pixelFormatInt = pixelFormatList[i].formatInt;
            break;
        }
    }
    if (!hasPixelFormat) {
        return resList;
    }

    int fd = open(path.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        printf("fail to open device. fd = %d\n", fd);
        return resList;
    }
    std::set<std::string> resSet;
    struct v4l2_frmsizeenum frmsize;
    frmsize.pixel_format = pixelFormatInt;
    frmsize.index = 0;
    while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
        std::string res;
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE || frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE){
            res = Strings::format(16, "%d*%d", frmsize.discrete.width, frmsize.discrete.height);
            int index = res.find('\0');
            resSet.insert(res.substr(0, index));
        }
        frmsize.index++;
    }
    close(fd);
    resList = std::vector<std::string>(resSet.begin(), resSet.end());
    std::sort(resList.begin(), resList.end(), [](const std::string &res1, const std::string &res2){
            std::vector<std::string> params1 = Strings::split(res1, "*");
            std::vector<std::string> params2 = Strings::split(res2, "*");
            return std::atoi(params1[0].c_str()) * std::atoi(params1[1].c_str()) >
                    std::atoi(params2[0].c_str()) * std::atoi(params2[1].c_str());
            });
    return resList;
}

int Camera::openPath(const std::string &path, const std::string &format, const std::string &res)
{
    fd = openDevice(path);
    if (fd < 0) {
        return -3;
    }
    usleep(500000);
    /* set format */
    std::vector<std::string> resList = Strings::split(res, "*");
    int w = std::atoi(resList[0].c_str());
    int h = std::atoi(resList[1].c_str());
    if (Camera::setFormat(w, h, format) == false) {
        printf("failed to setFormat.\n");
        return -4;
    }
    /* attach shared memory */
    if (attachSharedMemory() == false) {
        printf("fail to attachSharedMemory\n");
        return -5;
    }
    /* start sample */
    if (startSample() == false) {
        std::cout<<"fail to sample"<<std::endl;
        return -6;
    }
    return 0;
}

int Camera::start(const std::string &path, const std::string &format, const std::string &res)
{
    if (format.empty()) {
        printf("Camera::start: empty format\n");
        return -7;
    }
    if (res.empty()) {
        printf("Camera::start: empty resolution\n");
        return -7;
    }
    devPath = path;
    return openPath(devPath, format, res);
}

int Camera::start(unsigned short vid, unsigned short pid, const std::string &pixelFormat, int resIndex)
{
    /* enumerate */
    std::vector<Camera::Property> devList = Camera::enumerate();
    if (devList.empty()) {
        return -1;
    }
    /* get path by vid pid */
    Camera::Property dev;
    for (std::size_t i = 0; i < devList.size(); i++) {
        if (devList[i].vendorID == vid && devList[i].productID == pid) {
            dev = devList[i];
        }
    }
    if (dev.path.empty()) {
        return -2;
    }
    std::cout<<"dev path:"<<dev.path<<std::endl;
    /* get pixel format list */
    std::vector<PixelFormat> pixelFormatList = Camera::getPixelFormatList(dev.path);
    if (pixelFormatList.empty()) {
        return -3;
    }
    /* get resolution */
    std::vector<std::string> resList = Camera::getResolutionList(dev.path, pixelFormat);
    if (resList.empty()) {
        return -4;
    }
    if (resIndex >= resList.size()) {
        resIndex = 0;
    }
    resolutionMap[pixelFormat] = resList;
    devPath = dev.path;
    return openPath(dev.path, pixelFormat, resList[resIndex]);
}

void Camera::stop()
{
    stopSample();
    /* clear */
    closeDevice();

    usleep(1000000);
    return;
}

void Camera::clear()
{
    formatList.clear();
    resolutionMap.clear();
    return;
}

void Camera::restart(const std::string &format, const std::string &res)
{
    stop();
    start(devPath, format, res);
    return;
}

void Camera::setParam(unsigned int controlID, int value)
{
    v4l2_queryctrl queryctrl;
    queryctrl.id = controlID;
    if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
       if (errno != EINVAL) {
          return;
       } else {
          std::cout<<"ERROR :: Unable to set property (NOT SUPPORTED)\n";
          return;
       }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
       std::cout<<"ERROR :: Unable to set property (DISABLED).\n";
       return;
    } else {
        v4l2_control control{controlID, value};
        if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
            std::cout<<"Failed to set property.";
            return;
        }
        control.value = 0;
        if (ioctl(fd, VIDIOC_G_CTRL, &control) == -1) {
            std::cout<<"Failed to get property.";
        }
    }
    return;
}

int Camera::getParam(unsigned int controlID)
{
    v4l2_control ctrl{controlID, 0};
    if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) == -1) {
        return -1;
    }
    return ctrl.value;
}

void Camera::setWhiteBalanceMode(int value)
{
    setParam(V4L2_CID_AUTO_WHITE_BALANCE, value);
}

int Camera::getWhiteBalanceMode()
{
    return getParam(V4L2_CID_AUTO_WHITE_BALANCE);
}

void Camera::setWhiteBalanceTemperature(int value)
{
    setParam(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
    return;
}

int Camera::getWhiteBalanceTemperature()
{
    return getParam(V4L2_CID_WHITE_BALANCE_TEMPERATURE);
}

void Camera::setBrightnessMode(int value)
{
    setParam(V4L2_CID_AUTOBRIGHTNESS, value);
}

int Camera::getBrightnessMode()
{
    return getParam(V4L2_CID_AUTOBRIGHTNESS);
}

void Camera::setBrightness(int value)
{
    setParam(V4L2_CID_BRIGHTNESS, value);
}

int Camera::getBrightness()
{
    return getParam(V4L2_CID_BRIGHTNESS);
}

void Camera::setContrast(int value)
{
    setParam(V4L2_CID_CONTRAST, value);
}

int Camera::getContrast()
{
    return getParam(V4L2_CID_CONTRAST);
}

void Camera::setSaturation(int value)
{
    setParam(V4L2_CID_SATURATION, value);
}

int Camera::getSaturation()
{
    return getParam(V4L2_CID_SATURATION);
}

void Camera::setHue(int value)
{
    setParam(V4L2_CID_HUE, value);
}

int Camera::getHue()
{
    return getParam(V4L2_CID_HUE);
}

void Camera::setSharpness(int value)
{
    setParam(V4L2_CID_SHARPNESS, value);
}

int Camera::getSharpness()
{
    return getParam(V4L2_CID_SHARPNESS);
}

void Camera::setBacklightCompensation(int value)
{
    setParam(V4L2_CID_BACKLIGHT_COMPENSATION, value);
}

int Camera::getBacklightCompensation()
{
    return getParam(V4L2_CID_BACKLIGHT_COMPENSATION);
}

void Camera::setGamma(int value)
{
    setParam(V4L2_CID_GAMMA, value);
}

int Camera::getGamma()
{
    return getParam(V4L2_CID_GAMMA);
}

void Camera::setExposureMode(int value)
{
    setParam(V4L2_CID_EXPOSURE_AUTO, value);
}

int Camera::getExposureMode()
{
    return getParam(V4L2_CID_EXPOSURE_AUTO);
}

void Camera::setExposure(int value)
{
    setParam(V4L2_CID_EXPOSURE, value);
}

int Camera::getExposure()
{
    return getParam(V4L2_CID_EXPOSURE);
}

void Camera::setExposureAbsolute(int value)
{
    setParam(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

int Camera::getExposureAbsolute()
{
    return getParam(V4L2_CID_EXPOSURE_ABSOLUTE);
}

void Camera::setAutoGain(int value)
{
    setParam(V4L2_CID_AUTOGAIN, value);
}

int Camera::getAutoGain()
{
    return getParam(V4L2_CID_AUTOGAIN);
}

void Camera::setGain(int value)
{
    setParam(V4L2_CID_GAIN, value);
}

int Camera::getGain()
{
    return getParam(V4L2_CID_GAIN);
}

void Camera::setPowerLineFrequence(int value)
{
    setParam(V4L2_CID_POWER_LINE_FREQUENCY, value);
}

int Camera::getFrequency()
{
    return getParam(V4L2_CID_POWER_LINE_FREQUENCY);
}

void Camera::setDefaultParam()
{
    setWhiteBalanceMode(0);
    setWhiteBalanceTemperature(4600);
    setBrightnessMode(0);
    setBrightness(0);
    setContrast(32);
    setSaturation(64);
    setHue(0);
    setSharpness(3);
    setBacklightCompensation(0);
    setGamma(200);
    setExposureMode(V4L2_EXPOSURE_MANUAL);
    setExposureAbsolute(1500);
    setAutoGain(1);
    setGain(0);
    setPowerLineFrequence(V4L2_CID_POWER_LINE_FREQUENCY_50HZ);
    return;
}

void Camera::setParam(const Camera::DeviceParam &param)
{
    setWhiteBalanceMode(param.whiteBalanceMode);
    setWhiteBalanceTemperature(param.whiteBalanceTemperature);
    setBrightnessMode(param.brightnessMode);
    setBrightness(param.brightness);
    setContrast(param.contrast);
    setSaturation(param.saturation);
    setHue(param.hue);
    setSharpness(param.sharpness);
    setBacklightCompensation(param.backlightCompensation);
    setGamma(param.gamma);
    setExposureMode(param.exposureMode);
    setExposureAbsolute(param.exposureAbsolute);
    setAutoGain(param.autoGain);
    setGain(param.gain);
    setPowerLineFrequence(param.powerLineFrequence);
    return;
}
