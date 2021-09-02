#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    camera = new Camera(this);
    camera->enumDevice();
    /* device */
    ui->deviceComboBox->addItems(camera->getDevicePath());
    connect(ui->deviceComboBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
        this, [=](const QString &path){
         ui->formatComboBox->clear();
         ui->formatComboBox->addItems(camera->getFormat(path));
         ui->resolutionComboBox->clear();
         ui->resolutionComboBox->addItems(camera->getResolution(path));
         camera->setDevice(path);
    });
    /* format */
    QString path = camera->getCurrentDevicePath();
    ui->formatComboBox->addItems(camera->getFormat(path));
    connect(ui->formatComboBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            camera, &Camera::resetFormat);
    /* resolution */
    ui->resolutionComboBox->addItems(camera->getResolution(path));
    connect(ui->resolutionComboBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            camera, &Camera::resetResolution);
    connect(camera, &Camera::relay, this, &MainWindow::process);
    /* brightness */
    ui->brightnessSlider->setRange(-64, 64);
    connect(ui->brightnessSlider, &QSlider::valueChanged, camera, &Camera::setBrightness);
    /* white balance */
    ui->whiteBalanceSlider->setRange(0, 6500);
    connect(ui->whiteBalanceSlider, &QSlider::valueChanged, camera, &Camera::setWhiteBalanceTemperature);
    ui->whiteBalanceComboBox->addItems(QStringList{"MANUAL","AUTO","INCANDESCENT",
                                                   "FLUORESCENT","FLUORESCENT_H",
                                                   "HORIZON","DAYLIGHT","FLASH","CLOUDY","SHADE"});
    connect(ui->whiteBalanceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            camera, &Camera::setWhiteBalanceMode);
    /* contrast */
    ui->contrastSlider->setRange(0, 64);
    connect(ui->contrastSlider, &QSlider::valueChanged, camera, &Camera::setContrast);
    /* saturation */
    ui->saturationSlider->setRange(0, 128);
    connect(ui->saturationSlider, &QSlider::valueChanged, camera, &Camera::setSaturation);
    /* hue */
    ui->hueSlider->setRange(-64, 64);
    connect(ui->hueSlider, &QSlider::valueChanged, camera, &Camera::setHue);
    /* sharpness */
    ui->sharpnessSlider->setRange(0, 64);
    connect(ui->sharpnessSlider, &QSlider::valueChanged, camera, &Camera::setSharpness);
    /* backlight compensation */
    ui->backlightCompensationSlider->setRange(0, 6000);
    connect(ui->backlightCompensationSlider, &QSlider::valueChanged, camera, &Camera::getBacklightCompensation);
    /* gamma */
    ui->gammaSlider->setRange(0, 1000);
    connect(ui->gammaSlider, &QSlider::valueChanged, camera, &Camera::setGamma);
    /* exposure */
    ui->exposureComboBox->addItems(QStringList{"AUTO", "MANUAL", "SHUTTER_PRIORITY", "APERTURE_PRIORITY"});
    connect(ui->exposureComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            camera, &Camera::setExposureMode);
    ui->exposureSlider->setRange(-256, 256);
    connect(ui->exposureSlider, &QSlider::valueChanged, camera, &Camera::setExposure);
    ui->exposureAbsoluteSlider->setRange(0, 3200);
    connect(ui->exposureAbsoluteSlider, &QSlider::valueChanged, camera, &Camera::setExposureAbsolute);
    /* gain */
    ui->gainSlider->setRange(0, 100);
    connect(ui->gainSlider, &QSlider::valueChanged, camera, &Camera::setGain);
    /* frequency */
    ui->frequenceComboBox->addItems(QStringList{"DISABLED", "50HZ", "60HZ", "AUTO"});
    connect(ui->frequenceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            camera, &Camera::setFrequency);
    /* rotate */
    connect(ui->rotateSlider, &QSlider::valueChanged,
            camera, &Camera::setRotate);
    /* zoom */
    ui->zoomSlider->setRange(0, 256);
    connect(ui->zoomSlider, &QSlider::valueChanged, camera, &Camera::setZoom);
    /* focus */
    //void setFocusMode(unsigned int value);
    ui->focusComboBox->addItems(QStringList{"AUTO", "NORMAL", "MACRO", "INFINITY"});
    connect(ui->frequenceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            camera, &Camera::setFocusMode);
    /* update params */
    connect(ui->updateBtn, &QPushButton::clicked, camera, &Camera::getParams);
    connect(camera, &Camera::getParamsFinished, this, [=](const QMap<QString, int> &params){
        ui->whiteBalanceComboBox->setCurrentIndex(params["WhiteBalanceMode"]);
        ui->whiteBalanceSlider->setValue(params["WhiteBalanceTemperature"]);
        /* brightness */
        ui->brightnessSlider->setValue(params["Brightness"]);
        /* contrast */
        ui->contrastSlider->setValue(params["Contrast"]);
        /* saturation */
        ui->saturationSlider->setValue(params["Saturation"]);
        /* hue */
        ui->hueSlider->setValue(params["Hue"]);
        /* sharpness */
        ui->sharpnessSlider->setValue(params["Sharpness"]);
        /* backlight compensation */
        ui->backlightCompensationSlider->setValue(params["BacklightCompensation"]);
        /* gamma */
        ui->gammaSlider->setValue(params["Gamma"]);
        /* exposure */
        ui->exposureComboBox->setCurrentIndex(params["ExposureMode"]);
        ui->exposureSlider->setValue(params["Exposure"]);
        ui->exposureAbsoluteSlider->setValue(params["ExposureAbsolute"]);
        /* gain */
        ui->gainSlider->setValue(params["Gain"]);
        /* frequency */
        ui->frequenceComboBox->setCurrentIndex(params["Frequency"]);
        /* zoom */
        /* focus */
        ui->focusComboBox->setCurrentIndex(params["FocusMode"]);
    });
    /* set default */
    connect(ui->defaultBtn, &QPushButton::clicked, camera, &Camera::setDefaultParams);
    /* start */
    connect(ui->startBtn, &QPushButton::clicked, camera, &Camera::wakeUp);
    connect(ui->stopBtn, &QPushButton::clicked, camera, &Camera::pause);
    if (ui->deviceComboBox->currentText().isEmpty() ||
            ui->formatComboBox->currentText().isEmpty() ||
            ui->resolutionComboBox->currentText().isEmpty()) {
        QMessageBox::warning(this, "v4l2", "No device", QMessageBox::Ok);
    } else {
        camera->start(ui->deviceComboBox->currentText(),
                      ui->formatComboBox->currentText(),
                      ui->resolutionComboBox->currentText());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::process(const QImage &img)
{
    if (img.isNull()) {
        qDebug()<<"invalid image";
        return;
    }
    //QPixmap pixmap = QPixmap::fromImage(img.mirrored(true, true));
    QPixmap pixmap = QPixmap::fromImage(img);
    ui->cameralabel->updatePixmap(pixmap);
    return;
}
