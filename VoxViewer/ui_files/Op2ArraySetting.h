#ifndef OP2ARRAYSETTING_H
#define OP2ARRAYSETTING_H

#include <QDialog>
#include <QVector3D>

namespace Ui {
class Op2ArraySettingDlg;
}

class Op2ArraySettingDlg : public QDialog {
    Q_OBJECT

public:
    explicit Op2ArraySettingDlg(QWidget* parent);
    ~Op2ArraySettingDlg();

    void setArrayX(int x);
    void setArrayY(int y);
    void setVoxX(double x);
    void setVoxY(double y);

    void hideVox();

    void showCheckHideFileOpen();
    bool isCheckHideFileOpen();

    int    arrayX();
    int    arrayY();
    double voxX();
    double voxY();

private:
    Ui::Op2ArraySettingDlg* ui;
};

#endif    // OP2ARRAYSETTING_H
