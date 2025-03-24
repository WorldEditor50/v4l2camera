#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    methodName("none")
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    /* device */
    std::vector<Camera::Property> devices = Camera::Device::enumerate();
    if (devices.empty()) {
        QMessageBox::warning(this, "Notice", "no device");
        return;
    }
    for (std::size_t i = 0; i < devices.size(); i++) {
        ui->deviceComboBox->addItem(QString::fromStdString(devices[i].path));
    }
    QString devPath = QString::fromStdString(devices[0].path);
    ui->deviceComboBox->setCurrentText(devPath);

    connect(ui->deviceComboBox, &QComboBox::currentTextChanged,
        this, &MainWindow::onDeviceChanged);
    /* pixel format */
    std::vector<Camera::PixelFormat> pixelFormatList = Camera::Device::getPixelFormatList(devices[0].path);
    for (std::size_t i = 0; i < pixelFormatList.size(); i++) {
        ui->formatComboBox->addItem(QString::fromStdString(pixelFormatList[i].formatString));
    }
    ui->formatComboBox->setCurrentText(CAMERA_PIXELFORMAT_JPEG);
    connect(ui->formatComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::onPixelFormatChanged);
    /* resolution */
    std::vector<std::string> res = Camera::Device::getResolutionList(devices[0].path, CAMERA_PIXELFORMAT_JPEG);
    for (std::size_t i = 0; i < res.size(); i++) {
        ui->resolutionComboBox->addItem(QString::fromStdString(res[i]));
    }
    connect(ui->resolutionComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::onResolutionChanged);

    connect(ui->enumDeviceBtn, &QPushButton::clicked,
            this, &MainWindow::enumerateDevice);

    /* process */
    ui->methodComboBox->addItems(QStringList{"none", "canny", "laplace", "yolov5"});
    connect(ui->methodComboBox, &QComboBox::currentTextChanged, this, [=](const QString &name){
        methodName = name;
    });
    methodName = "none";
    ui->methodComboBox->setCurrentText(methodName);

    camera = new Camera::Device(Camera::Decode_SYNC, [this](int h, int w, int c, unsigned char* data){
        if (c == 3) {
            if (methodName == "canny") {
                Imageprocess::canny(h, w, data);
            } else if (methodName == "laplace") {
                Imageprocess::laplace(h, w, data);
            } else if (methodName == "yolov5") {
                Imageprocess::yolov5(h, w, data);
            }
            emit sendImage(QImage(data, w, h, QImage::Format_RGB888));
        } else if (c == 4) {
            emit sendImage(QImage(data, w, h, QImage::Format_ARGB32));
        }

    });
    camera->start(devices[0].path, CAMERA_PIXELFORMAT_JPEG, res[0]);
    connect(this, &MainWindow::sendImage,
            this, &MainWindow::updateImage, Qt::QueuedConnection);
    dialog = new SettingDialog(camera, this);
    connect(ui->settingBtn, &QPushButton::clicked, dialog, &SettingDialog::show);
}

MainWindow::~MainWindow()
{
    if (camera != nullptr) {
        delete camera;
        camera = nullptr;
    }
    delete ui;
}

void MainWindow::updateImage(const QImage &img)
{
    if (img.isNull()) {
        qDebug()<<"invalid image";
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(img.scaled(ui->cameralabel->size()));
    ui->cameralabel->setPixmap(pixmap);
    return;
}

void MainWindow::enumerateDevice()
{
    disconnect(ui->deviceComboBox, &QComboBox::currentTextChanged,
        this, &MainWindow::onDeviceChanged);

    std::vector<Camera::Property> devices = Camera::Device::enumerate();
    if (devices.empty()) {
        QMessageBox::warning(this, "Notice", "no device");
        return;
    }
    for (std::size_t i = 0; i < devices.size(); i++) {
        ui->deviceComboBox->addItem(QString::fromStdString(devices[i].path));
    }
    QString devPath = QString::fromStdString(devices[0].path);
    ui->deviceComboBox->setCurrentText(devPath);

    connect(ui->deviceComboBox, &QComboBox::currentTextChanged,
        this, &MainWindow::onDeviceChanged);
    return;
}

void MainWindow::onDeviceChanged(const QString &path)
{
    camera->stop();
    camera->clear();
    /* pixel format */
    disconnect(ui->formatComboBox, &QComboBox::currentTextChanged,
               this, &MainWindow::onPixelFormatChanged);

    ui->formatComboBox->clear();
    std::vector<Camera::PixelFormat> pixelFormatList = Camera::Device::getPixelFormatList(path.toStdString());
    for (std::size_t i = 0; i < pixelFormatList.size(); i++) {
        ui->formatComboBox->addItem(QString::fromStdString(pixelFormatList[i].formatString));
    }
    connect(ui->formatComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::onPixelFormatChanged);
    /* resolution */
    ui->resolutionComboBox->clear();
    disconnect(ui->resolutionComboBox, &QComboBox::currentTextChanged,
               this, &MainWindow::onResolutionChanged);

    std::vector<std::string> res = Camera::Device::getResolutionList(path.toStdString(),
                                                             CAMERA_PIXELFORMAT_JPEG);
    for (std::size_t i = 0; i < res.size(); i++) {
        ui->resolutionComboBox->addItem(QString::fromStdString(res[i]));
    }
    connect(ui->resolutionComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::onResolutionChanged);

    /* open camera */
    camera->start(path.toStdString(),
                  ui->formatComboBox->currentText().toStdString(),
                  ui->resolutionComboBox->currentText().toStdString());
    return;
}

void MainWindow::onPixelFormatChanged(const QString &format)
{
    camera->restart(format.toStdString(),
                    ui->resolutionComboBox->currentText().toStdString());
    return;
}

void MainWindow::onResolutionChanged(const QString &res)
{
    camera->restart(ui->formatComboBox->currentText().toStdString(),
                    res.toStdString());
    return;
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    if (camera != nullptr) {
        camera->stop();
    }
    return;
}
