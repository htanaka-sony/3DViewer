#include "Op2ArraySetting.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>
#include "ui_Op2ArraySetting.h"

Op2ArraySettingDlg::Op2ArraySettingDlg(QWidget* parent) : QDialog(parent), ui(new Ui::Op2ArraySettingDlg)
{
    ui->setupUi(this);

    ui->checkBox_hide_in_file_open->hide();
    adjustSize();
    resize(minimumSizeHint());
}

Op2ArraySettingDlg::~Op2ArraySettingDlg()
{
    delete ui;
}

void Op2ArraySettingDlg::setArrayX(int x)
{
    ui->spinBox_array_x->setValue(x);
}

void Op2ArraySettingDlg::setArrayY(int y)
{
    ui->spinBox_array_y->setValue(y);
}

void Op2ArraySettingDlg::setVoxX(double x)
{
    ui->doubleSpinBox_vox_x->setValue(x);
}

void Op2ArraySettingDlg::setVoxY(double y)
{
    ui->doubleSpinBox_vox_y->setValue(y);
}

void Op2ArraySettingDlg::hideVox()
{
    ui->doubleSpinBox_vox_x->setVisible(false);
    ui->doubleSpinBox_vox_y->setVisible(false);
    ui->label_vox_x->setVisible(false);
    ui->label_vox_y->setVisible(false);

    adjustSize();
    resize(minimumSizeHint());
}

void Op2ArraySettingDlg::showCheckHideFileOpen()
{
    ui->checkBox_hide_in_file_open->show();
    adjustSize();
    resize(minimumSizeHint());
}

bool Op2ArraySettingDlg::isCheckHideFileOpen()
{
    return ui->checkBox_hide_in_file_open->isChecked();
}

int Op2ArraySettingDlg::arrayX()
{
    return ui->spinBox_array_x->value();
}

int Op2ArraySettingDlg::arrayY()
{
    return ui->spinBox_array_y->value();
}

double Op2ArraySettingDlg::voxX()
{
    return ui->doubleSpinBox_vox_x->value();
}

double Op2ArraySettingDlg::voxY()
{
    return ui->doubleSpinBox_vox_y->value();
}
