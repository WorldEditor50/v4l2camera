#include "v4l2camera.h"

int V4l2Camera::maxDeviceNum = 8;
V4l2Camera::~V4l2Camera()
{
    clear();
    V4l2Allocator<unsigned long>::clear();
}
QString V4l2Camera::shellExecute(const QString& command)
{
    QString result = "";
    FILE *fpRead = popen(command.toUtf8(), "r");
    char buf[1024];
    memset(buf,'\0',sizeof(buf));
    while (fgets(buf, 1024-1, fpRead)!=NULL) {
        result = buf;
    }
    if (fpRead != NULL) {
        pclose(fpRead);
    }
    result.remove('\n');
    return result;
}

QString V4l2Camera::getVidPid(const QString &name)
{
    QString vid = shellExecute(QString("cat /sys/class/video4linux/%1/device/input/input*/id/vendor")
                                   .arg(name));
    QString pid = shellExecute(QString("cat /sys/class/video4linux/%1/device/input/input*/id/product")
                                   .arg(name));
    return QString("%1%2").arg(vid).arg(pid).toUpper();
}

QStringList V4l2Camera::findDevice(const QString &key, const std::function<bool(const QString &, const QString &)> &func)
{
    QStringList devicePathList;
    for (int i = 0; i < maxDeviceNum; i++) {
        QString video = QString("video%1").arg(i);
        if (QDir("/dev").exists(video) == false) {
            continue;
        }
        /* filter device */
        if (func(key, video) == false) {
            continue;
        }
        QString devPath = QString("/dev/%1").arg(video);
        int fd = open(devPath.toUtf8(), O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
            qDebug()<<"fail to open device.";
            continue;
        };
        /* check capability */
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc,0, sizeof(fmtdesc));
        fmtdesc.index = 0;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)<=-1) {
            continue;
        }
        devicePathList.append(devPath);
        close(fd);
    }
    return devicePathList;
}
QStringList V4l2Camera::findAllDevice()
{
    return findDevice("", [](const QString &, const QString &)->bool {
        return true;
    });
}

QStringList V4l2Camera::findDeviceByVidPid(const QString &vidpid)
{
    return findDevice(vidpid, [](const QString &vidpid, const QString &name)->bool {
        return vidpid == getVidPid(name);
    });
}

QStringList V4l2Camera::findDeviceByVid(const QString &vid)
{
    return findDevice(vid, [](const QString &vid, const QString &name)->bool {
        return vid == shellExecute(QString("cat /sys/class/video4linux/%1/device/input/input*/id/vendor")
                                       .arg(name)).toUpper();
    });
}

void V4l2Camera::openPath(const QString &path)
{
    int fd = open(path.toUtf8(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        qDebug()<<"fail to open device. fd = "<<fd;
        return;
    }
    devPath = path;
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap)<0) {
        qDebug()<<"Failed to set VIDIOC_QUERYCAP";
        return;
    }
    /*judge wherher or not to be a video-get device*/
    if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        qDebug()<<"Failed to set V4L2_BUF_TYPE_VIDEO_CAPTURE";
        return;
    }
    /*judge whether or not to supply the form of video stream*/
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        qDebug()<<"Failed to set V4L2_CAP_STREAMING";
        return;
    }
    /* get camera supported format */
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc,0, sizeof(fmtdesc));
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_frmsizeenum frmsize;
    while (1) {
        int ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);
        if (ret == -1) {
            printf("%s\n",strerror(errno));
            break;
        }
        QString description = QString((char*)fmtdesc.description);
        Format format;
        if (description.contains(FORMAT_JPEG)) {
            format.description = FORMAT_JPEG;
        } else if (description.contains(FORMAT_YUYV)) {
            format.description = FORMAT_YUYV;
        } else {
            format.description = description;
        }
        frmsize.pixel_format = fmtdesc.pixelformat;
        frmsize.index = 0;
        format.value = fmtdesc.pixelformat;
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
            if (format.isExist(frmsize.discrete.width, frmsize.discrete.height) == true) {
                frmsize.index++;
                continue;
            }
            Resolution res;
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE || frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE){
                res.width = frmsize.discrete.width;
                res.height = frmsize.discrete.height;
                format.resolutionList.push_back(res);
            }
            frmsize.index++;
        }
        std::sort(format.resolutionList.begin(), format.resolutionList.end(),
                  [](const Resolution &x1, const Resolution &x2){
                      if (x1.width * x1.height > x2.width * x2.height) {
                          return true;
                      }
                      return false;
                  });
        formatList.push_back(format);
        fmtdesc.index++;
    }
    close(fd);
    return;
}

QStringList V4l2Camera::getFormatList()
{
    QStringList formats;
    for (auto& x: formatList) {
        formats.push_back(x.description);
    }
    return formats;
}

QStringList V4l2Camera::getResolutionList()
{
    QSet<QString> res;
    for (auto& x: formatList) {
        for (auto& r : x.resolutionList) {
            res.insert(QString("%1*%2").arg(r.width).arg(r.height));
        }
    }
    QStringList resList = QStringList(res.toList());
    std::sort(resList.begin(), resList.end(), [](const QString &res1, const QString &res2){
        QStringList params1 = res1.split("*");
        QStringList params2 = res2.split("*");
        if (params1[0].toInt() * params1[1].toInt() >
            params2[0].toInt() * params2[1].toInt()) {
            return true;
        }
        return false;
    });
    return resList;
}

void V4l2Camera::start(const QString &format, const QString &res)
{
    fd = openDevice(devPath);
    if (fd < 0) {
        return;
    }
    QThread::usleep(500);
    /* set format */
    QStringList resList = res.split("*");
    int width = resList[0].toInt();
    int height = resList[1].toInt();
    if (V4l2Camera::setFormat(width, height, format) == false) {
        qDebug()<<"failed to setFormat.";
        return;
    }
    /* attach shared memory */
    if (attachSharedMemory() == false) {
        qDebug()<<"fail to attachSharedMemory";
        return;
    }
    /* start sample */
    if (startSample() == false) {
        std::cout<<"fail to sample"<<std::endl;
    }
    return;
}

void V4l2Camera::restart(const QString &format, const QString &res)
{
    if (isRunning.load()) {
        stopSample();
    }
    /* clear */
    closeDevice();
    start(format, res);
    return;
}

void V4l2Camera::clear()
{
    if (isRunning.load()) {
        stopSample();
    }
    /* clear */
    closeDevice();
    formatList.clear();
    return;
}

void V4l2Camera::process(int index, int size_)
{
    if (index >= sharedMemoryBlock.size()) {
        std::cout<<"invalid index"<<std::endl;
        return;
    }
    int width = param.width;
    int height = param.height;
    unsigned long len = width * height * 4;
    unsigned char* data = V4l2Allocator<unsigned char>::get(len);
    unsigned char *sharedMemoryPtr = (unsigned char*)sharedMemoryBlock[index].start;
    /* set format */
    if (formatDesc == FORMAT_JPEG) {
        libyuv::MJPGToARGB((const uint8*)sharedMemoryPtr, size_,
                           data, width * 4,
                           width, height, width, height);
    } else if(formatDesc == FORMAT_YUYV) {
        int alignedWidth = (width + 1) & ~1;
        libyuv::YUY2ToARGB(sharedMemoryPtr, alignedWidth * 2,
                           data, width * 4,
                           width, height);
    } else {
        V4l2Allocator<unsigned char>::recycle(len, data);
        qDebug()<<"decode failed."<<" format: "<<formatDesc;
        return;
    }
    /* process */
    QImage img = processImage(width, height, data);
    emit send(img);
    V4l2Allocator<unsigned char>::recycle(len, data);
    return;
}

void V4l2Camera::doSample()
{
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
            qDebug()<<"Failed to select";
            continue;
        }
        if (ret == 0) {
            qDebug()<<"select Timeout";
            continue;
        }
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        /* put cache from queue */
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            qDebug()<<"Failed to ioctl VIDIOC_DQBUF";
            continue;
        }
        /* process */
        process(buf.index, buf.bytesused);
        /* dequeue */
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            qDebug()<<"Failed to ioctl VIDIOC_QBUF";
            continue;
        }
    }
    return;
}

int V4l2Camera::openDevice(const QString &path)
{
    int fd = open(path.toUtf8(), O_RDWR, 0);
    if (fd < 0) {
        qDebug()<<"fail to open path.";
        return -1;
    };
    /* input */
    struct v4l2_input input;
    input.index = 0;
    if (ioctl(fd, VIDIOC_S_INPUT, &input) == -1) {
        qDebug()<<"fail to VIDIOC_S_INPUT:"<<fd;
        close(fd);
        return -1;
    }
    /* frame */
    v4l2_streamparm streamParam;
    memset(&streamParam, 0, sizeof(struct v4l2_streamparm));
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamParam.parm.capture.timeperframe.numerator = 1;
    streamParam.parm.capture.timeperframe.denominator = 2;
    if (ioctl(fd, VIDIOC_S_PARM, &streamParam) == -1) {
        qDebug()<<"failed to set frame";
    }
    return fd;
}

bool V4l2Camera::isSupportParam(unsigned int controlID)
{
    v4l2_queryctrl queryctrl;
    queryctrl.id = controlID;
    if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
        return false;
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        return false;
    }
    return true;
}

int V4l2Camera::getParam(unsigned int controlID)
{
    v4l2_control ctrl{controlID, 0};
    if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) == -1) {
        return -1;
    }
    return ctrl.value;
}

void V4l2Camera::setParam(unsigned int controlID, int value)
{
    v4l2_queryctrl queryctrl;
    queryctrl.id = controlID;
    if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
       if (errno != EINVAL) {
          return;
       } else {
          qDebug()<<"ERROR :: Unable to set property (NOT SUPPORTED)\n";
          return;
       }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
       qDebug()<<"ERROR :: Unable to set property (DISABLED).\n";
       return;
    } else {
        v4l2_control control{controlID, value};
        if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
            qDebug()<<"Failed to set property.";
            return;
        }
        control.value = 0;
        if (ioctl(fd, VIDIOC_G_CTRL, &control) == -1) {
            qDebug()<<"Failed to get property.";
        }
    }
    return;
}

bool V4l2Camera::checkCapability()
{
    /* check video decive driver capability */
    struct v4l2_capability 	cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        qDebug()<<"fail to ioctl VIDEO_QUERYCAP";
        close(fd);
        fd = -1;
        return false;
    }

    /*judge wherher or not to be a video-get device*/
    if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        qDebug()<<"The Current device is not a video capture device";
        close(fd);
        fd = -1;
        return false;
    }

    /*judge whether or not to supply the form of video stream*/
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        qDebug()<<"The Current device does not support streaming i/o";
        close(fd);
        fd = -1;
        return false;
    }
    return true;
}

bool V4l2Camera::setFormat(int w, int h, const QString &format)
{
    if (format.isEmpty()) {
        qDebug()<<"farmat is empty";
        return false;
    }
    /* set format */
    param.setFormat(w, h, format);
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = w;
    fmt.fmt.pix.height = h;
    fmt.fmt.pix.pixelformat = param.formatInt;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug()<<"Failed to set VIDIOC_S_FMT";
        return false;
    }
    formatDesc = format;
    return true;
}

void V4l2Camera::setAutoParam()
{
    setParam(V4L2_CID_FOCUS_AUTO, 0);
    setParam(V4L2_CID_HUE_AUTO, 1);
    setParam(V4L2_CID_AUTOGAIN, 1);
    setParam(V4L2_CID_AUTO_WHITE_BALANCE, V4L2_WHITE_BALANCE_AUTO);
    setParam(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_APERTURE_PRIORITY);
    return;
}

void V4l2Camera::setDefaultParam()
{
    if (checkCapability() == false) {
        return;
    }
    param.defaultParam();
    for (auto& x: param.control) {
        setParam(x.id, x.value);
    }
    return;
}

void V4l2Camera::saveParams(const QString &paramXml)
{
    for (auto& x: param.control) {
        x.value = getParam(x.id);
    }
    param.toXml(paramXml);
    return;
}

void V4l2Camera::loadParams(const QString &paramXml)
{
    if (param.loadFromXml(paramXml) == false) {
        return;
    }
    for (auto& x: param.control) {
        setParam(x.id, x.value);
    }
    return;
}
void V4l2Camera::setWhiteBalanceMode(int value)
{
    setParam(V4L2_CID_AUTO_WHITE_BALANCE, value);
}

int V4l2Camera::getWhiteBalanceMode()
{
    return getParam(V4L2_CID_AUTO_WHITE_BALANCE);
}

void V4l2Camera::setWhiteBalanceTemperature(int value)
{
    setParam(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
    return;
}

int V4l2Camera::getWhiteBalanceTemperature()
{
    return getParam(V4L2_CID_WHITE_BALANCE_TEMPERATURE);
}

void V4l2Camera::setBrightnessMode(int value)
{
    setParam(V4L2_CID_AUTOBRIGHTNESS, value);
}

int V4l2Camera::getBrightnessMode()
{
    return getParam(V4L2_CID_AUTOBRIGHTNESS);
}

void V4l2Camera::setBrightness(int value)
{
    setParam(V4L2_CID_BRIGHTNESS, value);
}

int V4l2Camera::getBrightness()
{
    return getParam(V4L2_CID_BRIGHTNESS);
}

void V4l2Camera::setContrast(int value)
{
    setParam(V4L2_CID_CONTRAST, value);
}

int V4l2Camera::getContrast()
{
    return getParam(V4L2_CID_CONTRAST);
}

void V4l2Camera::setSaturation(int value)
{
    setParam(V4L2_CID_SATURATION, value);
}

int V4l2Camera::getSaturation()
{
    return getParam(V4L2_CID_SATURATION);
}

void V4l2Camera::setHue(int value)
{
    setParam(V4L2_CID_HUE, value);
}

int V4l2Camera::getHue()
{
    return getParam(V4L2_CID_HUE);
}

void V4l2Camera::setSharpness(int value)
{
    setParam(V4L2_CID_SHARPNESS, value);
}

int V4l2Camera::getSharpness()
{
    return getParam(V4L2_CID_SHARPNESS);
}

void V4l2Camera::setBacklightCompensation(int value)
{
    setParam(V4L2_CID_BACKLIGHT_COMPENSATION, value);
}

int V4l2Camera::getBacklightCompensation()
{
    return getParam(V4L2_CID_BACKLIGHT_COMPENSATION);
}

void V4l2Camera::setGamma(int value)
{
    setParam(V4L2_CID_GAMMA, value);
}

int V4l2Camera::getGamma()
{
    return getParam(V4L2_CID_GAMMA);
}

void V4l2Camera::setExposureMode(int value)
{
    setParam(V4L2_CID_EXPOSURE_AUTO, value);
}

int V4l2Camera::getExposureMode()
{
    return getParam(V4L2_CID_EXPOSURE_AUTO);
}

void V4l2Camera::setExposure(int value)
{
    setParam(V4L2_CID_EXPOSURE, value);
}

int V4l2Camera::getExposure()
{
    return getParam(V4L2_CID_EXPOSURE);
}

void V4l2Camera::setExposureAbsolute(int value)
{
    setParam(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

int V4l2Camera::getExposureAbsolute()
{
    return getParam(V4L2_CID_EXPOSURE_ABSOLUTE);
}

void V4l2Camera::setGainMode(int value)
{
    setParam(V4L2_CID_AUTOGAIN, value);
}

int V4l2Camera::getGainMode()
{
    return getParam(V4L2_CID_AUTOGAIN);
}

void V4l2Camera::setGain(int value)
{
    setParam(V4L2_CID_GAIN, value);
}

int V4l2Camera::getGain()
{
    return getParam(V4L2_CID_GAIN);
}

void V4l2Camera::setFrequency(int value)
{
    setParam(V4L2_CID_POWER_LINE_FREQUENCY, value);
}

int V4l2Camera::getFrequency()
{
    return getParam(V4L2_CID_POWER_LINE_FREQUENCY);
}

bool V4l2Camera::attachSharedMemory()
{
    /* to request frame cache, contain requested counts*/
    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(reqbufs));
    /*the number of buffer*/
    reqbufs.count = mmapBlockCount;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbufs) == -1) {
        qDebug()<<"Fail to ioctl 'VIDIOC_REQBUFS'";
        close(fd);
        fd = -1;
        return false;
    }
    /* map kernel cache to user process */
    sharedMemoryBlock = QVector<SharedMemory>(mmapBlockCount);
    for (int i = 0; i < sharedMemoryBlock.size(); i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        /* check the information of the kernel cache requested */
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            qDebug()<<"Fail to ioctl : VIDIOC_QUERYBUF";
            close(fd);
            fd = -1;
            return false;
        }
        sharedMemoryBlock[i].length = buf.length;
        sharedMemoryBlock[i].start = (char *)mmap(NULL,
                                        buf.length,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        fd, buf.m.offset);
        if (sharedMemoryBlock[i].start == MAP_FAILED) {
            qDebug()<<"Fail to mmap";
            close(fd);
            fd = -1;
            return false;
        }
    }
    /* place the kernel cache to a queue */
    for (int i = 0; i < sharedMemoryBlock.size(); i++) {
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

void V4l2Camera::dettachSharedMemory()
{
    for (auto& x : sharedMemoryBlock) {
        if (munmap(x.start, x.length) == -1) {
            qDebug()<<"Failed to munmap";
        }
    }
    sharedMemoryBlock.clear();
    return;
}

bool V4l2Camera::startSample()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        qDebug()<<"Fail to ioctl 'VIDIOC_STREAMON";
        closeDevice();
        return false;
    }
    /* start thread */
    isRunning.store(1);
    future = QtConcurrent::run(this, &V4l2Camera::doSample);
    return true;
}

bool V4l2Camera::stopSample()
{
    isRunning.store(0);
    future.waitForFinished();
    v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        qDebug()<<"Fail to ioctl VIDIOC_STREAMOFF";
        return false;
    }
    return true;
}

void V4l2Camera::closeDevice()
{
    /* dettach shared memory */
    dettachSharedMemory();
    /* close device */
    close(fd);
    fd = -1;
    return;
}
