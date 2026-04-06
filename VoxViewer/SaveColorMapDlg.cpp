#include "SaveColorMapDlg.h"

#include "ResultCtrl.h"
#include "SettingCtrl.h"
#include "Vox3DForm.h"
#include "ui_SaveColorMap.h"
#include "ui_Vox3DForm.h"

#include <QDialog>
#include <QRegularExpression>

SaveColorMapDlg::SaveColorMapDlg(QWidget* parent, Ui::Vox3DForm* vox3d_ui, const QString& init_name)
    : QDialog(parent)
    , ui(new Ui::SaveColorMapDlg)
    , m_vox3d_ui(vox3d_ui)
{
    ui->setupUi(this);

    ui->lineEdit->setText(init_name);

    connect(ui->pushButton_allon, &QPushButton::clicked, this, [this]() {
        checkSaveMaxColor(true);
        checkSaveMinColor(true);
        checkSaveSteps(true);
        checkSaveRange(true);
        checkSaveGradient(true);
        checkSaveReverse(true);
    });

    connect(ui->pushButton_alloff, &QPushButton::clicked, this, [this]() {
        checkSaveMaxColor(false);
        checkSaveMinColor(false);
        checkSaveSteps(false);
        checkSaveRange(false);
        checkSaveGradient(false);
        checkSaveReverse(false);
    });
}

SaveColorMapDlg::~SaveColorMapDlg()
{
    delete ui;
}

QString SaveColorMapDlg::filename()
{
    return ui->lineEdit->text();
}

void SaveColorMapDlg::checkSaveMinColor(bool checked)
{
    ui->checkBox_min->setChecked(checked);
}

void SaveColorMapDlg::checkSaveMaxColor(bool checked)
{
    ui->checkBox_max->setChecked(checked);
}

void SaveColorMapDlg::checkSaveSteps(bool checked)
{
    ui->checkBox_steps->setChecked(checked);
}

void SaveColorMapDlg::checkSaveRange(bool checked)
{
    ui->checkBox_range->setChecked(checked);
}

void SaveColorMapDlg::checkSaveGradient(bool checked)
{
    ui->checkBox_gradient->setChecked(checked);
}

void SaveColorMapDlg::checkSaveReverse(bool checked)
{
    ui->checkBox_reverse->setChecked(checked);
}

bool SaveColorMapDlg::isSaveMinColor()
{
    return ui->checkBox_min->isChecked();
}

bool SaveColorMapDlg::isSaveMaxColor()
{
    return ui->checkBox_max->isChecked();
}

bool SaveColorMapDlg::isSaveSteps()
{
    return ui->checkBox_steps->isChecked();
}

bool SaveColorMapDlg::isSaveRange()
{
    return ui->checkBox_range->isChecked();
}

bool SaveColorMapDlg::isSaveGradient()
{
    return ui->checkBox_gradient->isChecked();
}

bool SaveColorMapDlg::isSaveReverse()
{
    return ui->checkBox_reverse->isChecked();
}

void SaveColorMapDlg::accept()
{
    QSet<QString> default_name;
    QSet<QString> exist_name;
    QSet<QString> exist_file_name;

    /// カラーマップ名
    auto model = (QStandardItemModel*)m_vox3d_ui->comboBox_result3d_custom_setting->model();
    for (int index = 0; index < model->rowCount(); ++index) {
        auto color_item = model->item(index);
        if (color_item) {
            if (color_item->text() == E2VIEWER_NAME) {
                default_name << color_item->text().toLower();
                continue;
            }
            int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
            if (is_custom == 0) {
                default_name << color_item->text().toLower();
            }
            if (is_custom == 1) {
                exist_name << color_item->text().toLower();
            }
        }
    }

    /// ファイル名
    QDir        color_map_dir(SettingCtrl::resultColorMapPath());
    QStringList fileList = color_map_dir.entryList(QStringList() << "*.csv", QDir::Files | QDir::NoDotAndDotDot);
    for (QString fileName : fileList) {
        if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
            fileName.chop(4);
        }
        exist_file_name << fileName.toLower();
    }

    QString name = ui->lineEdit->text().toLower();
    if (name.endsWith(".csv", Qt::CaseInsensitive)) {
        name.chop(4);
    }

    QRegularExpression re(R"([\\/:*?"<>|])");
    if (re.match(name).hasMatch()) {
        QMessageBox::warning(this, windowTitle(), "ファイルの禁則文字は使えません");
        return;
    }

    if (default_name.contains(name)) {
        QMessageBox::warning(this, windowTitle(), "標準の名称と重複するため設定できません\n名前を変えてください");
        return;
    }
    if (exist_name.contains(name) || exist_file_name.contains(name)) {
        if (QMessageBox::question(this, windowTitle(), "既に存在します。上書きしますか？") != QMessageBox::Yes) {
            return;
        }
    }

    QDialog::accept();
}
