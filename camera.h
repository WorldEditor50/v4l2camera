#ifndef CAMERA_H
#define CAMERA_H
#include "logger.hpp"
#include "v4l2device.h"
#include <QProcess>

class Camera : public QObject
{
    Q_OBJECT
public:
    static QStringList formatFilter;
private:
    int width;
    int height;
    QMap<QString, V4l2Device*> deviceMap;
    QMap<QString, CameraInfo> infoMap;
    QString currentDevPath;
    QString currentFormat;
    QString currentPidVid;
signals:
    void relay(const QImage &img);
    void getParamsFinished(const QMap<QString, int> &params);
public slots:
    void resetFormat(const QString &format);
    void resetResolution(const QString &res);
    void getParams();
public:
    explicit Camera(QObject *parent);
    ~Camera();
    /* init */
    int filterDevice(const std::string &vidpid, int fd);
    QString getPid();
    QString getVid();
    virtual int enumDevice();
    virtual void getDevice(int fd, const QString &devPath);
    void clear();
    bool openDevice(const QString &devPath);
    /* get */
    QStringList getFormat(const QString &devPath);
    QStringList getResolution(const QString &devPath);
    QStringList getDevicePath();
    QString getCurrentDevicePath();
    /* set */
    void setDevice(const QString &path);
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
    void setGainMode(int value);
    int getGainMode();
    void setGain(int value);
    int getGain();
    /* frequency */
    void setFrequency(int value);
    int getFrequency();
    /* rotate */
    void setRotate(int value);
    int getRotate();
    /* zoom */
    void setZoom(int value);
    /* focus */
    void setFocusMode(int value);
    int getFocusMode();
    /* set default params */
    void setDefaultParams();
    /* save params */
    void saveParams(const QString &fileName);
    void loadParams(const QString &fileName);
    /* start - stop */
    void start(const QString &devPath, const QString &format, const QString &res);
    void stop(const QString &devPath);
    void pause();
    void wakeUp();
};
#endif //CAMERA_H
