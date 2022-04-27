#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include "v4l2camera.h"

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(V4l2Camera *camera_, QWidget *parent = nullptr);
    ~SettingDialog();
    void updateParam();
    void setDefault();
    void dumpParam();
private:
    Ui::SettingDialog *ui;
    V4l2Camera *camera;
};

#endif // SETTINGDIALOG_H
