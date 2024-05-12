#include "mainwindow.h"
#include <QApplication>
#include "yolov5.h"
#include <QtConcurrent/QtConcurrent>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    /* load model */
    QtConcurrent::run([](){
        bool ret = Yolov5::instance().load("/home/galois/MySpace/model/yolov5s_6.0");
        if (ret == false) {
            QMessageBox::warning(nullptr, "Notice", "Failed to load model", QMessageBox::Ok);
        }
    });
    MainWindow w;
    w.show();
    return a.exec();
}
