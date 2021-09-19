#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    camera = new V4l2Camera(this);
    /* device */
    ui->deviceComboBox->addItems(camera->findAllDevice());
    camera->openPath(ui->deviceComboBox->currentText());
    connect(ui->deviceComboBox, &QComboBox::currentTextChanged,
        this, &MainWindow::updateDevice);
    connect(ui->enumDeviceBtn, &QPushButton::clicked, this, &MainWindow::enumDevice);
    /* format */
    ui->formatComboBox->addItems(camera->getFormatList());
    connect(ui->formatComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::updateFormat);
    /* resolution */
    ui->resolutionComboBox->addItems(camera->getResolutionList());
    connect(ui->resolutionComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::updateResolution);
    connect(camera, &V4l2Camera::send, this, &MainWindow::updateImage);
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
    connect(ui->defaultBtn, &QPushButton::clicked, this, &MainWindow::setDefault);

    /* process */
    camera->setProcessFunc([](int width, int height, unsigned char* data)->QImage{
        return Imageprocess::invoke(width, height, data);
        //return QImage(data, width, height, QImage::Format_ARGB32);
    });
    if (ui->deviceComboBox->currentText().isEmpty() ||
            ui->formatComboBox->currentText().isEmpty() ||
            ui->resolutionComboBox->currentText().isEmpty()) {
        //QMessageBox::warning(this, "v4l2", "No device", QMessageBox::Ok);
    } else {
        camera->start(ui->formatComboBox->currentText(),
                      ui->resolutionComboBox->currentText());
        updateParam();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateImage(const QImage &img)
{
    if (img.isNull()) {
        qDebug()<<"invalid image";
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(img);
    ui->cameralabel->updatePixmap(pixmap);
    return;
}

void MainWindow::enumDevice()
{
    disconnect(ui->deviceComboBox, &QComboBox::currentTextChanged,
        this, &MainWindow::updateDevice);
    ui->deviceComboBox->addItems(camera->findAllDevice());
    connect(ui->deviceComboBox, &QComboBox::currentTextChanged,
        this, &MainWindow::updateDevice);
}

void MainWindow::updateDevice(const QString &path)
{
    camera->clear();
    camera->openPath(path);
    disconnect(ui->formatComboBox, &QComboBox::currentTextChanged,
               this, &MainWindow::updateFormat);
    disconnect(ui->resolutionComboBox, &QComboBox::currentTextChanged,
               this, &MainWindow::updateResolution);
    ui->formatComboBox->clear();
    ui->resolutionComboBox->clear();
    ui->formatComboBox->addItems(camera->getFormatList());
    ui->resolutionComboBox->addItems(camera->getResolutionList());
    connect(ui->formatComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::updateFormat);
    connect(ui->resolutionComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::updateResolution);
    camera->start(ui->formatComboBox->currentText(),
                  ui->resolutionComboBox->currentText());
    updateParam();
    return;
}

void MainWindow::updateFormat(const QString &format)
{
    camera->restart(format, ui->resolutionComboBox->currentText());
    return;
}

void MainWindow::updateResolution(const QString &res)
{
    camera->restart(ui->formatComboBox->currentText(), res);
    return;
}

void MainWindow::updateParam()
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

void MainWindow::setDefault()
{
    camera->setDefaultParam();
    updateParam();
}
