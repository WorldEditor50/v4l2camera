#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include "v4l2camera.h"
#include "imageprocess.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void updateImage(const QImage &img);
    void enumDevice();
    void updateDevice(const QString &path);
    void updateFormat(const QString &format);
    void updateResolution(const QString &res);
    void updateParam();
    void setDefault();
private:
    Ui::MainWindow *ui;
    V4l2Camera *camera;
};

#endif // MAINWINDOW_H
