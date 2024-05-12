#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QPixmap>
#include "camera/camera.h"
#include "imageprocess.h"
#include "settingdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
signals:
    void sendImage(const QImage &img);
public slots:
    void updateImage(const QImage &img);
    void enumerateDevice();
    void onDeviceChanged(const QString &path);
    void onPixelFormatChanged(const QString &format);
    void onResolutionChanged(const QString &res);
protected:
    void closeEvent(QCloseEvent *ev) override;
private:
    Ui::MainWindow *ui;
    Camera *camera;
    SettingDialog *dialog;
    QString methodName;
};

#endif // MAINWINDOW_H
