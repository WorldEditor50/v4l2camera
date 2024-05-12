#include "settingdialog.h"
#include "ui_settingdialog.h"
#include <QDir>
#include <QFile>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>
#include <QMap>
#include <QTextStream>
#include <linux/videodev2.h>

SettingDialog::SettingDialog(Camera *camera_, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog),
    camera(camera_)
{
    ui->setupUi(this);
    setWindowTitle("camera parameters");
    /* brightness */
    ui->brightnessSlider->setRange(-64, 64);
    connect(ui->brightnessSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setBrightness(value);
    });
    /* white balance */
    ui->whiteBalanceSlider->setRange(0, 6500);
    connect(ui->whiteBalanceSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setWhiteBalanceTemperature(value);
    });
    ui->whiteBalanceComboBox->addItems(QStringList{"MANUAL","AUTO","INCANDESCENT",
                                                   "FLUORESCENT","FLUORESCENT_H",
                                                   "HORIZON","DAYLIGHT","FLASH","CLOUDY","SHADE"});
    connect(ui->whiteBalanceComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int value){
        camera->setWhiteBalanceMode(value);
    });
    /* contrast */
    ui->contrastSlider->setRange(0, 64);
    connect(ui->contrastSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setContrast(value);
    });
    /* saturation */
    ui->saturationSlider->setRange(0, 128);
    connect(ui->saturationSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setSaturation(value);
    });
    /* hue */
    ui->hueSlider->setRange(-64, 64);
    connect(ui->hueSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setHue(value);
    });
    /* sharpness */
    ui->sharpnessSlider->setRange(0, 64);
    connect(ui->sharpnessSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setSharpness(value);
    });
    /* backlight compensation */
    ui->backlightCompensationSlider->setRange(0, 6000);
    connect(ui->backlightCompensationSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setBacklightCompensation(value);
    });
    /* gamma */
    ui->gammaSlider->setRange(0, 1000);
    connect(ui->gammaSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setGamma(value);
    });
    /* exposure */
    ui->exposureComboBox->addItems(QStringList{"AUTO", "MANUAL", "SHUTTER_PRIORITY", "APERTURE_PRIORITY"});
    connect(ui->exposureComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int value){
        camera->setExposureMode(value);
    });
    ui->exposureAbsoluteSlider->setRange(0, 6400);
    connect(ui->exposureAbsoluteSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setExposureAbsolute(value);
    });
    /* gain */
    ui->gainSlider->setRange(0, 100);
    connect(ui->gainSlider, &QSlider::valueChanged, this, [=](int value){
        camera->setGain(value);
    });
    /* frequency */
    ui->frequenceComboBox->addItems(QStringList{"DISABLED", "50HZ", "60HZ", "AUTO"});
    connect(ui->frequenceComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int value){
        camera->setPowerLineFrequence(value);
    });
    /* set default */
    connect(ui->defaultBtn, &QPushButton::clicked,
            this, &SettingDialog::setDefault);
    /* dump default parameters */
    if (QDir(".").exists("camera_default_params.xml") == false) {
        saveParams("camera_default_params.xml");
    }
}

void SettingDialog::saveParams(const QString &fileName)
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
    auto insertElement = [&doc, this](const QString &key, const QString &value, QDomElement &parentElement) {
        QDomElement element = doc.createElement(key);
        element.appendChild(doc.createTextNode(value));
        parentElement.appendChild(element);
    };
    /* add node and element */
    QDomElement param = doc.createElement("Parameter");
    insertElement("WhiteBalanceMode",
                  QString::number(camera->getWhiteBalanceMode()), param);
    insertElement("WhiteBalanceTemperature",
                  QString::number(camera->getWhiteBalanceTemperature()), param);
    insertElement("BrightnessMode",
                  QString::number(camera->getBrightnessMode()), param);
    insertElement("Brightness",
                  QString::number(camera->getBrightness()), param);
    insertElement("Contrast",
                  QString::number(camera->getContrast()), param);
    insertElement("Saturation",
                  QString::number(camera->getSaturation()), param);
    insertElement("Hue",
                  QString::number(camera->getHue()), param);
    insertElement("Sharpness",
                  QString::number(camera->getSharpness()), param);
    insertElement("BacklightCompensation",
                  QString::number(camera->getBacklightCompensation()), param);
    insertElement("Gamma",
                  QString::number(camera->getGamma()), param);
    insertElement("ExposureMode",
                  QString::number(camera->getExposureMode()), param);
    insertElement("ExposureAbsolute",
                  QString::number(camera->getExposureAbsolute()), param);
    insertElement("AutoGain",
                  QString::number(camera->getAutoGain()), param);
    insertElement("Gain",
                  QString::number(camera->getGain()), param);
    insertElement("PowerLineFrequence",
                  QString::number(camera->getFrequency()), param);
    /* add node */
    root.appendChild(param);
    /* output */
    QTextStream out(&file);
    doc.save(out, 4);
    file.close();
    return;
}

bool SettingDialog::loadParams(const QString &fileName)
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
                        QString param = n.nodeName();
                        int value = n.toElement().text().toInt();
                        if (param == "WhiteBalanceMode") {
                            camera->setWhiteBalanceMode(value);
                        } else if (param == "WhiteBalanceTemperature") {
                            camera->setWhiteBalanceTemperature(value);
                        } else if (param == "BrightnessMode") {
                            camera->setBrightnessMode(value);
                        } else if (param == "Brightness") {
                            camera->setBrightness(value);
                        } else if (param == "Contrast") {
                            camera->setContrast(value);
                        } else if (param == "Saturation") {
                            camera->setSaturation(value);
                        } else if (param == "Hue") {
                            camera->setHue(value);
                        } else if (param == "Sharpness") {
                            camera->setSharpness(value);
                        } else if (param == "BacklightCompensation") {
                            camera->setBacklightCompensation(value);
                        } else if (param == "Gamma") {
                            camera->setGamma(value);
                        } else if (param == "ExposureMode") {
                            camera->setExposureMode(value);
                        } else if (param == "ExposureAbsolute") {
                            camera->setExposureAbsolute(value);
                        } else if (param == "AutoGain") {
                            camera->setAutoGain(value);
                        } else if (param == "Gain") {
                            camera->setGain(value);
                        } else if (param == "PowerLineFrequence") {
                            camera->setPowerLineFrequence(value);
                        }

                    }
                }
            }
        }
        node = node.nextSibling();
    }
    return true;
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::setDefault()
{
    loadParams("camera_default_params.xml");
    updateParam();
    return;
}

void SettingDialog::updateParam()
{
    /* brightness */
    ui->brightnessSlider->setValue(camera->getBrightness());
    /* white balance */
    ui->whiteBalanceSlider->setValue(camera->getWhiteBalanceTemperature());
    ui->whiteBalanceComboBox->setCurrentIndex(camera->getWhiteBalanceMode());
    /* contrast */
    ui->contrastSlider->setValue(camera->getContrast());
    /* saturation */
    ui->saturationSlider->setValue(camera->getSaturation());
    /* hue */
    ui->hueSlider->setValue(camera->getHue());
    /* sharpness */
    ui->sharpnessSlider->setValue(camera->getSharpness());
    /* backlight compensation */
    ui->backlightCompensationSlider->setValue(camera->getBacklightCompensation());
    /* gamma */
    ui->gammaSlider->setValue(camera->getGamma());
    /* exposure */
    ui->exposureComboBox->setCurrentIndex(camera->getExposureMode());
    ui->exposureAbsoluteSlider->setValue(camera->getExposureAbsolute());
    /* gain */
    ui->gainSlider->setValue(camera->getGain());
    /* frequency */
    ui->frequenceComboBox->setCurrentIndex(camera->getFrequency());
    return;
}
