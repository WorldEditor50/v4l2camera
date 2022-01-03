#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
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


    /* process */
    camera->setProcessFunc([this](unsigned char* data, int width, int height){
        QImage img = Imageprocess::laplace(width, height, data);
        //QImage img(data, width, height, QImage::Format_ARGB32);
        emit sendImage(img);
    });
    if (ui->deviceComboBox->currentText().isEmpty() ||
            ui->formatComboBox->currentText().isEmpty() ||
            ui->resolutionComboBox->currentText().isEmpty()) {
        //QMessageBox::warning(this, "v4l2", "No device", QMessageBox::Ok);
    } else {
        camera->start(ui->formatComboBox->currentText(),
                      ui->resolutionComboBox->currentText());
    }
    connect(this, &MainWindow::sendImage, this, &MainWindow::updateImage);
    dialog = new SettingDialog(camera, this);
    connect(ui->settingBtn, &QPushButton::clicked, dialog, &SettingDialog::show);
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
    ui->cameralabel->setPixmap(pixmap);
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
