#include "v4l2param.h"
#include <QDebug>

V4l2Param::V4l2Param()
{
    control.insert("WhiteBalanceMode", Control(V4L2_CID_AUTO_WHITE_BALANCE, 0));
    control.insert("WhiteBalanceTemperature", Control(V4L2_CID_WHITE_BALANCE_TEMPERATURE, 4600));
    control.insert("BrightnessMode", Control(V4L2_CID_AUTOBRIGHTNESS, 0));
    control.insert("Brightness", Control(V4L2_CID_BRIGHTNESS, 0));
    control.insert("Contrast", Control(V4L2_CID_CONTRAST, 32));
    control.insert("Saturation", Control(V4L2_CID_SATURATION, 64));
    control.insert("Hue", Control(V4L2_CID_HUE, 0));
    control.insert("Sharpness", Control(V4L2_CID_SHARPNESS, 3));
    control.insert("BacklightCompensation", Control(V4L2_CID_BACKLIGHT_COMPENSATION, 0));
    control.insert("Gamma", Control(V4L2_CID_GAMMA, 200));
    control.insert("ExposureMode", Control(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL));
    control.insert("ExposureAbsolute", Control(V4L2_CID_EXPOSURE_ABSOLUTE, 1500));
    control.insert("AutoGain", Control(V4L2_CID_AUTOGAIN, 1));
    control.insert("Gain", Control(V4L2_CID_GAIN, 0));
    control.insert("PowerLineFrequence", Control(V4L2_CID_POWER_LINE_FREQUENCY, V4L2_CID_POWER_LINE_FREQUENCY_50HZ));
}

void V4l2Param::setFormat(int w, int h, const QString &format)
{
    width = w;
    height = h;
    if (format == FORMAT_JPEG) {
        formatInt = V4L2_PIX_FMT_MJPEG;
    } else if (format == FORMAT_YUYV) {
        formatInt = V4L2_PIX_FMT_YUYV;
    }
    return;
}

void V4l2Param::setDefault()
{
    control["WhiteBalanceMode"] = Control(V4L2_CID_AUTO_WHITE_BALANCE, 0);
    control["WhiteBalanceTemperature"] = Control(V4L2_CID_WHITE_BALANCE_TEMPERATURE, 4600);
    control["BrightnessMode"] = Control(V4L2_CID_AUTOBRIGHTNESS, 0);
    control["Brightness"] = Control(V4L2_CID_BRIGHTNESS, 0);
    control["Contrast"] = Control(V4L2_CID_CONTRAST, 32);
    control["Saturation"] = Control(V4L2_CID_SATURATION, 64);
    control["Hue"] = Control(V4L2_CID_HUE, 0);
    control["Sharpness"] = Control(V4L2_CID_SHARPNESS, 2);
    control["BacklightCompensation"] = Control(V4L2_CID_BACKLIGHT_COMPENSATION, 0);
    control["Gamma"] = Control(V4L2_CID_GAMMA, 0);
    control["ExposureMode"] = Control(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_APERTURE_PRIORITY);
    control["ExposureAbsolute"] = Control(V4L2_CID_EXPOSURE_ABSOLUTE, 1500);
    control["AutoGain"] = Control(V4L2_CID_AUTOGAIN, 1);
    control["Gain"] = Control(V4L2_CID_GAIN, 4);
    control["PowerLineFrequence"] = Control(V4L2_CID_POWER_LINE_FREQUENCY, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);
    return;
}

void V4l2Param::toXml(const QString &fileName)
{
    /* open file */
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly|QFile::Truncate)) {
        return;
    }
    QDomDocument doc;
    /* instruction */
    QDomProcessingInstruction instruction;
    instruction=doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    /* root */
    QDomElement root = doc.createElement("Camera");
    doc.appendChild(root);
    auto insertElement = [&doc](const QString &key, const QString &value, QDomElement &parentElement) {
        QDomElement element = doc.createElement(key);
        element.appendChild(doc.createTextNode(value));
        parentElement.appendChild(element);
    };
    /* add node and element */
    QDomElement param = doc.createElement("Parameter");
    for (auto it = control.begin(); it != control.end(); it++) {
        insertElement(it.key(), QString::number(it.value().value), param);
    }
    /* add node */
    root.appendChild(param);
    /* output */
    QTextStream out(&file);
    doc.save(out, 4);
    file.close();
    return;
}

QMap<QString, int> V4l2Param::toMap()
{
    QMap<QString, int> data;
    for (auto it = control.begin(); it != control.end(); it++) {
        data[it.key()] = it.value().value;
    }
    return data;
}

bool V4l2Param::loadFromXml(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }
    QDomDocument doc;
    if (!doc.setContent(&file))  {
        file.close();
        return false;
    }
    file.close();

    /* root */
    QDomElement root = doc.documentElement();
    QDomNode node = root.firstChild();
    while (!node.isNull()) {
        if (node.isElement()) {
            QDomElement e = node.toElement();
            if (e.tagName() == "Parameter") {
                QDomNodeList list=e.childNodes();
                for (int i = 0; i < list.count(); i++) {
                    QDomNode n = list.at(i);
                    if (n.isElement() == true) {
                        control[n.nodeName()].value = n.toElement().text().toInt();
                    }
                }
            }
        }
        node = node.nextSibling();
    }
    return true;
}

void V4l2Param::show()
{
    for (auto it = control.begin(); it != control.end(); it++) {
        qDebug()<<it.key()<<":"<<it.value().value;
    }
}
