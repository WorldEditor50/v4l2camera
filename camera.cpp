#include "camera.h"
#include <QDebug>

QStringList Camera::formatFilter = {"YUYV", "JPEG"};

void Camera::resetFormat(const QString &format)
{
    if (format.isEmpty() == true) {
        return;
    }
    if (format == currentFormat) {
        return;
    }
    currentFormat = format;
    V4l2Device *device = deviceMap[currentDevPath];
    disconnect(device, &V4l2Device::send, this, &Camera::relay);
    deviceMap[currentDevPath]->restart(currentFormat, width, height);
    connect(device, &V4l2Device::send, this, &Camera::relay, Qt::QueuedConnection);
}

void Camera::resetResolution(const QString &res)
{
    if (res.isEmpty() == true) {
        return;
    }
    if (res == QString("%1*%2").arg(width).arg(height)) {
        return;
    }
    QStringList resList = res.split("*");
    if (resList.isEmpty() == false) {
        width = resList[0].toInt();
        height = resList[1].toInt();
        V4l2Device *device = deviceMap[currentDevPath];
        disconnect(device, &V4l2Device::send, this, &Camera::relay);
        deviceMap[currentDevPath]->restart(currentFormat, width, height);
        connect(device, &V4l2Device::send, this, &Camera::relay, Qt::QueuedConnection);
    }
    return;
}

void Camera::getParams()
{
    QMap<QString, int> deviceParams = deviceMap[currentDevPath]->updateParams();
    if (deviceParams.isEmpty() == false) {
        emit getParamsFinished(deviceParams);
    }
    return;
}

Camera::Camera(QObject *parent):
    QObject(parent),
    width(0),
    height(0),
    currentDevPath(""),
    currentFormat("")
{

}

Camera::~Camera()
{
    clear();
}
char* shellExecute(const char* command)
{
    char* result = "";
    FILE *fpRead;
    fpRead = popen(command, "r");
    char buf[1024];
    memset(buf,'\0',sizeof(buf));
    while (fgets(buf,1024-1, fpRead)!=NULL) {
        result = buf;
    }
    if (fpRead!=NULL) {
        pclose(fpRead);
    }
    return result;
}

char *getDeviceId(const char* index, int what)
{
    char cmd[70];
    memset(cmd, 0, sizeof(cmd));
    if (what == 0){
        sprintf(cmd, "%s%s%s","cat /sys/class/video4linux/",index,"/device/input/input*/id/vendor");
        printf("%s\n",cmd);
    }else{
        sprintf(cmd, "%s%s%s","cat /sys/class/video4linux/",index,"/device/input/input*/id/product");
        printf("%s\n",cmd);
    }
    char* result = shellExecute(cmd);
    return result;
}
int Camera::filterDevice(const std::string &vidpid, int fd)
{
    if (fd < 0) {
        return -1;
    }
    /* check capability */
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc,0, sizeof(fmtdesc));
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)<=-1) {
        return -2;
    }
    /* check vid : todo */
    return 0;
}

QString Camera::getPid()
{
    QProcess proc;
    proc.execute(QString("cat /sys/class/video4linux/%1/device/input/input*/id/product").arg(currentDevPath));
    proc.waitForFinished(3000);
    return QString(proc.readAllStandardOutput());
}

QString Camera::getVid()
{
    QProcess proc;
    proc.execute(QString("cat /sys/class/video4linux/%1/device/input/input*/id/vendor").arg(currentDevPath));
    proc.waitForFinished(3000);
    return QString(proc.readAllStandardOutput());
}

int Camera::enumDevice()
{
    int ret = -1;
    DIR *dir;
    struct dirent *ptr;
    if ((dir = opendir("/dev")) == NULL) {
        qDebug()<<"failed to open /dev";
        return ret;
    }
    while ((ptr = readdir(dir))!=NULL) {
        if (ptr->d_type == DT_CHR) {
            if (strstr(ptr->d_name, "video")) {
                QString devPath = QString("/dev/%1").arg(ptr->d_name);
                printf("ptr->d_name:%s\n", ptr->d_name);
                int fd = open(devPath.toUtf8(), O_RDWR | O_NONBLOCK,0);
                if(fd < 0) {
                    fprintf(stderr, "%s open err \n", ptr->d_name);
                    continue;
                };
                getDevice(fd, devPath);
                close(fd);
            }
        }
    }
    deviceMap.clear();
    return ret;
}

void Camera::getDevice(int fd, const QString &devPath)
{
    struct v4l2_capability 	cap;
    memset(&cap, 0, sizeof(cap));
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap)<0) {
        LOG(Log::ERROR, "VIDIOC_QUERYCAP fail");
        return;
    }
    /*judge wherher or not to be a video-get device*/
    if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
        LOG(Log::ERROR, "not V4L2_BUF_TYPE_VIDEO_CAPTURE");
        return;
    }
    /*judge whether or not to supply the form of video stream*/
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOG(Log::ERROR, "not V4L2_CAP_STREAMING");
        return;
    }
    /* get camera device's name */
    CameraInfo dev;
    dev.cardName = QString((char*)cap.card);
    dev.devPath = devPath;
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc,0, sizeof(fmtdesc));
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /* get formats */
    struct v4l2_frmsizeenum frmsize;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
        QString description = QString((char*)fmtdesc.description);
        PixelFormat pixelFormatNode;
        if (description.contains(FORMAT_YUYV)) {
            pixelFormatNode.formatString = FORMAT_YUYV;
        }  else if (description.contains(FORMAT_JPEG)) {
            pixelFormatNode.formatString = FORMAT_JPEG;
        } else {
            pixelFormatNode.formatString = description;
        }
        frmsize.pixel_format = fmtdesc.pixelformat;
        frmsize.index = 0;
        pixelFormatNode.formatInt = fmtdesc.pixelformat;
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
            if (pixelFormatNode.isExist(frmsize.discrete.width, frmsize.discrete.height) == true) {
                frmsize.index++;
                continue;
            }
            FrameData frmSizeNode;
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE || frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE){
                frmSizeNode.width = frmsize.discrete.width;
                frmSizeNode.height = frmsize.discrete.height;
                pixelFormatNode.resolutionList.push_back(frmSizeNode);
            }
            frmsize.index++;
        }
        std::sort(pixelFormatNode.resolutionList.begin(), pixelFormatNode.resolutionList.end(),
                  [](const FrameData &x1, const FrameData &x2){
            if (x1.width * x1.height > x2.width * x2.height) {
                return true;
            }
            return false;
        });
        dev.formatList.push_back(pixelFormatNode);
        fmtdesc.index++;
    }
    if (dev.formatList.size() > 0) {
        infoMap[devPath] = dev;
    }
    return;
}

void Camera::clear()
{
    saveParams("cameraparams.xml");
    infoMap.clear();
    for (V4l2Device* p : deviceMap) {
        if (p != nullptr) {
            p->stopSample();
        }
        delete p;
    }
    deviceMap.clear();
}

bool Camera::openDevice(const QString &devPath)
{
    int fd = V4l2Device::openDevice(devPath);
    if (fd < 0) {
        LOG(Log::ERROR, "failed to open device");
        return false;
    }
    deviceMap[devPath] = new V4l2Device(devPath, fd);
    return true;
}

QStringList Camera::getFormat(const QString &devPath)
{
    QStringList formats;
    for (auto& x: infoMap[devPath].formatList) {
        formats.push_back(x.formatString);
    }
    return formats;
}

QStringList Camera::getResolution(const QString &devPath)
{
    QSet<QString> res;
    for (auto& x: infoMap[devPath].formatList) {
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

QStringList Camera::getDevicePath()
{
    QStringList devpath;
    for (auto& x: infoMap) {
        devpath.push_back(x.devPath);
    }
    if (devpath.isEmpty() == false) {
        currentDevPath = devpath[0];
    } else {
        LOG(Log::ERROR, "No device");
    }
    return devpath;
}

QString Camera::getCurrentDevicePath()
{
    return currentDevPath;
}

void Camera::setDevice(const QString &path)
{
    if (currentDevPath == path) {
        return;
    }
    /* disconnect */
    V4l2Device *device = deviceMap[currentDevPath];
    disconnect(device, &V4l2Device::send, this, &Camera::relay);
    /* stop current camera */
    device->stopSample();
    /* start new camera */
    start(path, currentFormat, QString("%1*%2").arg(width).arg(height));
    return;
}

void Camera::setWhiteBalanceMode(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_AUTO_WHITE_BALANCE, value);
}

int Camera::getWhiteBalanceMode()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_AUTO_WHITE_BALANCE);
}

void Camera::setWhiteBalanceTemperature(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
}

int Camera::getWhiteBalanceTemperature()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_WHITE_BALANCE_TEMPERATURE);
}

void Camera::setBrightnessMode(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_AUTOBRIGHTNESS, value);
}

int Camera::getBrightnessMode()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_AUTOBRIGHTNESS);
}

void Camera::setBrightness(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_BRIGHTNESS, value);
}

int Camera::getBrightness()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_BRIGHTNESS);
}

void Camera::setContrast(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_CONTRAST, value);
}

int Camera::getContrast()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_CONTRAST);
}

void Camera::setSaturation(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_SATURATION, value);
}

int Camera::getSaturation()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_SATURATION);
}

void Camera::setHue(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_HUE, value);
}

int Camera::getHue()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_HUE);
}

void Camera::setSharpness(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_SHARPNESS, value);
}

int Camera::getSharpness()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_SHARPNESS);
}

void Camera::setBacklightCompensation(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_BACKLIGHT_COMPENSATION, value);
}

int Camera::getBacklightCompensation()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_BACKLIGHT_COMPENSATION);
}

void Camera::setGamma(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_GAMMA, value);
}

int Camera::getGamma()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_GAMMA);
}

void Camera::setExposureMode(int value)
{   
    deviceMap[currentDevPath]->setParam(V4L2_CID_EXPOSURE_AUTO, value);
}

int Camera::getExposureMode()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_EXPOSURE_AUTO);
}

void Camera::setExposure(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_EXPOSURE, value);
}

int Camera::getExposure()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_EXPOSURE);
}

void Camera::setExposureAbsolute(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

int Camera::getExposureAbsolute()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_EXPOSURE_ABSOLUTE);
}

void Camera::setGainMode(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_AUTOGAIN, value);
}

int Camera::getGainMode()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_AUTOGAIN);
}

void Camera::setGain(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_GAIN, value);
}

int Camera::getGain()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_GAIN);
}

void Camera::setFrequency(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_POWER_LINE_FREQUENCY, value);
}

int Camera::getFrequency()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_POWER_LINE_FREQUENCY);
}

void Camera::setRotate(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_HFLIP, value);
}

void Camera::setZoom(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_ZOOM_ABSOLUTE, value);
}

void Camera::setFocusMode(int value)
{
    deviceMap[currentDevPath]->setParam(V4L2_CID_AUTO_FOCUS_RANGE, value);
    return;
}

int Camera::getFocusMode()
{
    return deviceMap[currentDevPath]->getParam(V4L2_CID_AUTO_FOCUS_RANGE);
}

void Camera::setDefaultParams()
{
    deviceMap[currentDevPath]->setDefaultParam();
}

void Camera::saveParams(const QString &fileName)
{
    deviceMap[currentDevPath]->saveParams(fileName);
}

void Camera::loadParams(const QString &fileName)
{
    deviceMap[currentDevPath]->loadParams(fileName);
}

void Camera::start(const QString &devPath, const QString &format, const QString &res)
{
    /* open */
    if (openDevice(devPath) == false) {
        return;
    }
    currentDevPath = devPath;
    currentFormat = format;
    V4l2Device* device = deviceMap[devPath];
    if (device == nullptr) {
        qDebug()<<"empty device.";
        return;
    }
    /* set default param */
//    if (device->setDefaultParam() == false) {
//        qDebug()<<"failed to setDefaultParam.";
//        return;
//    }
    /* get params */
    getParams();
    /* set format */
    QStringList resList = res.split("*");
    width = resList[0].toInt();
    height = resList[1].toInt();
    if (device->setFormat(width, height, format) == false) {
        qDebug()<<"failed to setFormat.";
        return;
    }
    /* relay */
    connect(device, &V4l2Device::send, this, &Camera::relay, Qt::QueuedConnection);
    /* attach shared memory */
    if (device->attachSharedMemory() == false) {
        qDebug()<<"fail to attachSharedMemory";
        return;
    }
    /* start sample */
    if (device->startSample() == false) {
        std::cout<<"fail to sample"<<std::endl;
    }
    return;
}

void Camera::stop(const QString &devPath)
{ 
    V4l2Device *device = deviceMap[devPath];
    disconnect(device, &V4l2Device::send, this, &Camera::relay);
    device->stopSample();
    device->deleteLater();
    deviceMap.remove(devPath);
    return;
}

void Camera::pause()
{
    deviceMap[currentDevPath]->pauseSample();
    return;
}

void Camera::wakeUp()
{
    deviceMap[currentDevPath]->wakeUpSample();
    return;
}
