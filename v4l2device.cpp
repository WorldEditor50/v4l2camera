#include "v4l2device.h"

V4l2Device::~V4l2Device()
{
    clear();
}

void V4l2Device::process(int index, int size_)
{
    if (state == SAMPLE_PAUSE) {
        return;
    }
    if (index >= memoryVec.size()) {
        std::cout<<"invalid index"<<std::endl;
        return;
    }
    int width = param.width;
    int height = param.height;
    unsigned long len = width * height * 4;
    unsigned char* data = ImageDataPool::get(len);
    uint8 *sharedMemoryPtr = (uint8*)memoryVec[index].start;
    /* set format */
    if (formatString == FORMAT_JPEG) {
        libyuv::MJPGToARGB((const uint8*)sharedMemoryPtr, size_,
                           data, width * 4,
                           width, height, width, height);
    } else if(formatString == FORMAT_YUYV) {
        int alignedWidth = (width + 1) & ~1;
        libyuv::YUY2ToARGB(sharedMemoryPtr, alignedWidth * 2,
                           data, width * 4,
                           width, height);
    } else {
        qDebug()<<"decode failed."<<" format: "<<formatString;
    }
    /* process */
    emit send(Imageprocess::invoke(width, height, data));
    ImageDataPool::recycle(len, data);
    return;
}

void V4l2Device::sampling()
{
    qDebug()<<"enter sampling function.";
    while (true) {
        {
            QMutexLocker locker(&mutex);
            while (state == SAMPLE_PAUSE) {
                condit.wait(&mutex);
            }
            if (state == SAMPLE_TERMINATE) {
                break;
            }
        }
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
        /* process */
        process(buf.index, buf.bytesused);
        /* dequeue */
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("0 Fail to ioctl 'VIDIOC_QBUF'");
            continue;
        }
    }
    qDebug()<<"leave sampling function.";
    return;
}

int V4l2Device::openDevice(const QString &path)
{
    if (path.isEmpty()) {
        qDebug()<<"dev path is empty";
        return -1;
    }
    int fd = open(path.toUtf8(), O_RDWR, 0);
    if (fd < 0) {
        qDebug()<<"fail to open device."<<path;
        LOG(Log::ERROR, "fail to open device.");
        return -1;
    };
    struct v4l2_input input;
    input.index = 0;
    if (ioctl(fd, VIDIOC_S_INPUT, &input) == -1) {
        qDebug()<<"fail to VIDIOC_S_INPUT:"<<fd;
        LOG(Log::ERROR, "Failed to ioctl VIDIOC_S_INPUT");
        close(fd);
        return -1;
    }
    return fd;
}

bool V4l2Device::isSupportParam(unsigned int controlID)
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

int V4l2Device::getParam(unsigned int controlID)
{
    v4l2_control ctrl{controlID, 0};
    if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) == -1) {
        return -1;
    }
    return ctrl.value;
}

void V4l2Device::setParam(unsigned int controlID, int value)
{
    v4l2_queryctrl queryctrl;
    queryctrl.id = controlID;
    if (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
       if (errno != EINVAL) {
          return;
       } else {
          std::cout<< "ERROR :: Unable to set property (NOT SUPPORTED)\n"<<std::endl;
          return;
       }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
       std::cout << "ERROR :: Unable to set property (DISABLED).\n";
       return;
    } else {
        v4l2_control control{controlID, value};
        if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
            std::cout << "Failed to set property." << std::endl;
        }
    }
    return;
}

bool V4l2Device::checkCapability()
{
    /* check video decive driver capability */
    struct v4l2_capability 	cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        fprintf(stderr, "fail to ioctl VIDEO_QUERYCAP \n");
        LOG(Log::ERROR, "Failed to ioctl VIDEO_QUERYCAP");
        close(fd);
        fd = -1;
        return false;
    }

    /*judge wherher or not to be a video-get device*/
    if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        fprintf(stderr, "The Current device is not a video capture device \n");
        LOG(Log::ERROR, "The Current device is not a video capture device");
        close(fd);
        fd = -1;
        return false;
    }

    /*judge whether or not to supply the form of video stream*/
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("The Current device does not support streaming i/o\n");
        LOG(Log::ERROR, "The Current device does not support streaming i/o");
        close(fd);
        fd = -1;
        return false;
    }
    return true;
}

bool V4l2Device::setFormat(int w, int h, const QString &format)
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
        perror("VIDIOC_S_FMT set err");
        LOG(Log::ERROR, "Fail to ioctl 'VIDIOC_S_FMT'");
        return false;
    }
    formatString = format;
    return true;
}

void V4l2Device::setAutoParam()
{
    setParam(V4L2_CID_FOCUS_AUTO, 0);
    setParam(V4L2_CID_HUE_AUTO, 1);
    setParam(V4L2_CID_AUTOGAIN, 1);
    setParam(V4L2_CID_AUTO_WHITE_BALANCE, V4L2_WHITE_BALANCE_AUTO);
    setParam(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_APERTURE_PRIORITY);
    return;
}

bool V4l2Device::setDefaultParam()
{
    if (checkCapability() == false) {
        return false;
    }
    param.setDefault();
    for (auto& x: param.control) {
        setParam(x.id, x.value);
    }
    /* frame */
    v4l2_streamparm streamParam;
    memset(&streamParam, 0, sizeof(struct v4l2_streamparm));
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamParam.parm.capture.timeperframe.numerator = 1;
    streamParam.parm.capture.timeperframe.denominator = 2;
    ioctl(fd, VIDIOC_S_PARM, &streamParam);
    return true;
}

QMap<QString, int> V4l2Device::updateParams()
{
    for (auto& x: param.control) {
        x.value = getParam(x.id);
    }
    return param.toMap();
}

void V4l2Device::saveParams(const QString &paramXml)
{
    for (auto& x: param.control) {
        x.value = getParam(x.id);
    }
    param.toXml(paramXml);
    return;
}

void V4l2Device::loadParams(const QString &paramXml)
{
    if (param.loadFromXml(paramXml) == false) {
        return;
    }
    for (auto& x: param.control) {
        setParam(x.id, x.value);
    }
    return;
}

bool V4l2Device::attachSharedMemory()
{
    /* to request frame cache, contain requested counts*/
    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(reqbufs));
    /*the number of buffer*/
    reqbufs.count = mmapBlockCount;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbufs) == -1) {
        perror("Fail to ioctl 'VIDIOC_REQBUFS'");
        LOG(Log::ERROR, "Fail to ioctl 'VIDIOC_REQBUFS'");
        close(fd);
        fd = -1;
        return false;
    }
    /* map kernel cache to user process */
    memoryVec = QVector<SharedMemBlock>(mmapBlockCount);
    for (int i = 0; i < memoryVec.size(); i++) {
        //stand for a frame
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        /*check the information of the kernel cache requested*/
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("Fail to ioctl : VIDIOC_QUERYBUF");
            LOG(Log::ERROR, "Fail to mmap");
            close(fd);
            fd = -1;
            return false;
        }
        memoryVec[i].length = buf.length;
        memoryVec[i].start = (char *)mmap(NULL,
                                        buf.length,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        fd, buf.m.offset);
        if (memoryVec[i].start == MAP_FAILED) {
            perror("Fail to mmap");
            LOG(Log::ERROR, "Fail to mmap");
            close(fd);
            fd = -1;
            return false;
        }
    }
    /* place the kernel cache to a queue */
    for (int i = 0; i < memoryVec.size(); i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Fail to ioctl 'VIDIOC_QBUF'");
            LOG(Log::ERROR, "Fail to ioctl 'VIDIOC_QBUF");
            clear();
            return false;
        }
    }
    return true;
}

void V4l2Device::dettachSharedMemory()
{
    for (auto& x : memoryVec) {
        if (munmap(x.start, x.length) == -1) {
            perror("Fail to munmap");
        }
    }
    memoryVec.clear();
    return;
}

bool V4l2Device::startSample()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("VIDIOC_STREAMON");
        LOG(Log::ERROR, "Fail to ioctl 'VIDIOC_STREAMON");
        clear();
        return false;
    }
    /* start thread */
    future = QtConcurrent::run(this, &V4l2Device::sampling);
    wakeUpSample();
    return true;
}

bool V4l2Device::stopSample()
{
    if (state == SAMPLE_RUNNING) {
        pauseSample();
    }
    terminateSample();
    future.waitForFinished();
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
        LOG(Log::ERROR, "Fail to ioctl 'VIDIOC_STREAMOFF");
        return false;
    }
    return true;
}

void V4l2Device::pauseSample()
{
    QMutexLocker locker(&mutex);
    if (state != SAMPLE_PAUSE) {
        state = SAMPLE_PAUSE;
    }
    return;
}

void V4l2Device::wakeUpSample()
{
    QMutexLocker locker(&mutex);
    if (state != SAMPLE_RUNNING) {
        state = SAMPLE_RUNNING;
        condit.wakeAll();
    }
    return;
}

void V4l2Device::terminateSample()
{
    QMutexLocker locker(&mutex);
    if (state != SAMPLE_TERMINATE) {
        state = SAMPLE_TERMINATE;
        condit.wakeAll();
    }
    return;
}

void V4l2Device::clear()
{
    /* stop sample */
    stopSample();
    /* dettach shared memory */
    dettachSharedMemory();
    /* close device */
    close(fd);
    fd = -1;
    /* release data memory pool */
    ImageDataPool::clear();
}

void V4l2Device::restart(const QString &format, int width, int height)
{
    /* stop sample */
    if (stopSample() == false) {
        qDebug()<<"fail to stop sample.";
        return;
    }
    /* clear */
    clear();
    /* open */
    fd = openDevice(path);
    if (fd < 0) {
        qDebug()<<"open device.";
        return;
    }
    QThread::usleep(500);
    /* set param */
    if (setDefaultParam() == false) {
        qDebug()<<"fail to setDefaultParam.";
        return;
    }
    /* set format */
    if (setFormat(width, height, format) == false) {
        qDebug()<<"fail to setFormat.";
        return;
    }
    /* attach shared memory */
    if (attachSharedMemory() == false) {
        qDebug()<<"fail to attachSharedMemory";
        return;
    }
    /* start sample */
    if (startSample() == false) {
        qDebug()<<"fail to start sample";
    }
    return;
}
