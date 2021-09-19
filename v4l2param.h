#ifndef V4L2PARAM_H
#define V4L2PARAM_H
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QMap>
#include <linux/videodev2.h>
#define FORMAT_YUYV "YUYV"
#define FORMAT_JPEG "JPEG"


struct Control
{
    int id;
    int value;
    Control():id(0), value(0){}
    Control(int id_, int value_):id(id_), value(value_){}
};

class V4l2Param
{
public:
    int width;
    int height;
    int formatInt;
    /* pid vid */
    int pid;
    int vid;
    /* control */
    QMap<QString, Control> control;
public:
    V4l2Param();
    ~V4l2Param(){}
    void setFormat(int w, int h, const QString &format);
    void defaultParam();
    void toXml(const QString& fileName);
    QMap<QString, int> toMap();
    bool loadFromXml(const QString& fileName);
    void show();
};
#endif // V4L2PARAM_H
