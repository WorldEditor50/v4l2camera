#include "settingdialog.h"
#include "ui_settingdialog.h"

SettingDialog::SettingDialog(V4l2Camera *camera_, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog),
    camera(camera_)
{
    ui->setupUi(this);
    setWindowTitle("camera parameters");
    /* brightness */
    ui->brightnessSlider->setRange(-64, 64);
    connect(ui->brightnessSlider, &QSlider::valueChanged, camera, &V4l2Camera::setBrightness);
    /* white balance */
    ui->whiteBalanceSlider->setRange(0, 6500);
    connect(ui->whiteBalanceSlider, &QSlider::valueChanged, camera, &V4l2Camera::setWhiteBalanceTemperature);
    ui->whiteBalanceComboBox->addItems(QStringList{"MANUAL","AUTO","INCANDESCENT",
                                                   "FLUORESCENT","FLUORESCENT_H",
                                                   "HORIZON","DAYLIGHT","FLASH","CLOUDY","SHADE"});
    connect(ui->whiteBalanceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            camera, &V4l2Camera::setWhiteBalanceMode);
    /* contrast */
    ui->contrastSlider->setRange(0, 64);
    connect(ui->contrastSlider, &QSlider::valueChanged, camera, &V4l2Camera::setContrast);
    /* saturation */
    ui->saturationSlider->setRange(0, 128);
    connect(ui->saturationSlider, &QSlider::valueChanged, camera, &V4l2Camera::setSaturation);
    /* hue */
    ui->hueSlider->setRange(-64, 64);
    connect(ui->hueSlider, &QSlider::valueChanged, camera, &V4l2Camera::setHue);
    /* sharpness */
    ui->sharpnessSlider->setRange(0, 64);
    connect(ui->sharpnessSlider, &QSlider::valueChanged, camera, &V4l2Camera::setSharpness);
    /* backlight compensation */
    ui->backlightCompensationSlider->setRange(0, 6000);
    connect(ui->backlightCompensationSlider, &QSlider::valueChanged, camera, &V4l2Camera::getBacklightCompensation);
    /* gamma */
    ui->gammaSlider->setRange(0, 1000);
    connect(ui->gammaSlider, &QSlider::valueChanged, camera, &V4l2Camera::setGamma);
    /* exposure */
    ui->exposureComboBox->addItems(QStringList{"AUTO", "MANUAL", "SHUTTER_PRIORITY", "APERTURE_PRIORITY"});
    connect(ui->exposureComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            camera, &V4l2Camera::setExposureMode);
    ui->exposureAbsoluteSlider->setRange(0, 6400);
    connect(ui->exposureAbsoluteSlider, &QSlider::valueChanged, camera, &V4l2Camera::setExposureAbsolute);
    /* gain */
    ui->gainSlider->setRange(0, 100);
    connect(ui->gainSlider, &QSlider::valueChanged, camera, &V4l2Camera::setGain);
    /* frequency */
    ui->frequenceComboBox->addItems(QStringList{"DISABLED", "50HZ", "60HZ", "AUTO"});
    connect(ui->frequenceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            camera, &V4l2Camera::setFrequency);
    /* set default */
    connect(ui->defaultBtn, &QPushButton::clicked, this, &SettingDialog::setDefault);
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::setDefault()
{
    camera->setDefaultParam();
    updateParam();
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
