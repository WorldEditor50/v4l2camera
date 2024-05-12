#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include "camera/camera.h"

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(Camera *camera_, QWidget *parent = nullptr);
    ~SettingDialog();
    void saveParams(const QString &fileName);
    bool loadParams(const QString &fileName);
    void updateParam();
    void setDefault();
    void dumpParam();
private:
    Ui::SettingDialog *ui;
    Camera *camera;
};

#endif // SETTINGDIALOG_H
