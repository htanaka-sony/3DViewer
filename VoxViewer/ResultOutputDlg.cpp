#include "ResultOutputDlg.h"
#include "ClippingCtrl.h"
#include "FileExplorerDialog.h"
#include "MyOpenGLWidget.h"
#include "ResultCtrl.h"
#include "SettingGuiCtrl.h"
#include "UtilityCtrl.h"
#include "Vox3DForm.h"

#include "Op2ArraySetting.h"
#include "OutputFileThread.h"
#include "ReadOp2.h"
#include "ReadOpt.h"
#include "ReadVoxel.h"

#include "Scene/Node.h"
#include "Scene/SceneView.h"
#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QGroupBox>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QVBoxLayout>

#include "ui_ResultOutputDlg.h"

#define RESULT_OUTPUT_TARGET_LIST_TARGET 0
#define RESULT_OUTPUT_TARGET_LIST_FILENAME 1
#define RESULT_OUTPUT_TARGET_LIST_TYPE 2
#define RESULT_OUTPUT_TARGET_LIST_FOLDER 3
#define RESULT_OUTPUT_TARGET_LIST_ARRAY_X 4
#define RESULT_OUTPUT_TARGET_LIST_ARRAY_Y 5
#define RESULT_OUTPUT_TARGET_LIST_RELATED_FILE 6

#define RESULT_OUTPUT_TARGET_LIST_FILE_FULLPATH RESULT_OUTPUT_TARGET_LIST_FILENAME

class NoElideDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        // 標準の背景と選択状態のみ描画
        opt.text      = "";
        QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        // テキスト描画
        QString text     = index.data(Qt::DisplayRole).toString();
        QRect   textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        textRect.adjust(2, 0, 0, 0);

        painter->save();
        painter->setFont(opt.font);
        painter->setPen(opt.palette.color(QPalette::Text));
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, text);
        painter->restore();
    }
};

class ComboBoxDialog : public QDialog {
public:
    explicit ComboBoxDialog(const QString& title, QWidget* parent = nullptr) : QDialog(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // QComboBox を作成
        m_combo = new CustomCombo(this);
        mainLayout->addWidget(m_combo);

        // ボタンを配置するための QHBoxLayout を作成
        QHBoxLayout* buttonLayout = new QHBoxLayout();

        // OK ボタンを作成
        QPushButton* okButton = new QPushButton("OK", this);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(okButton);

        // キャンセルボタンを作成
        QPushButton* cancelButton = new QPushButton("キャンセル", this);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        buttonLayout->addWidget(cancelButton);

        // ボタンレイアウトをメインレイアウトに追加
        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);
        setWindowTitle(title);
    }

    CustomCombo* m_combo;    // QComboBoxをメンバ変数として保持
};

ResultOutputDlg::ResultOutputDlg(Vox3DForm* parent, MyOpenGLWidget* gl_widget)
    : m_3DForm(parent)
    , m_gl_widget(gl_widget)
    , QDialog(parent)
    , ui(new Ui::ResultOutputDlg)
{
    ui->setupUi(this);

    ui->tabWidget->setCustomTabBar();
    ui->tabWidget_base->setCustomTabBar();

    ui->radioButton_target_current->setChecked(true);
    // ui->lineEdit_target_folder->setEnabled(false);
    connect(ui->radioButton_target_current, &QRadioButton::toggled, this, [this]() { updateEnableTargetList(); });
    connect(ui->radioButton_target_current_disp, &QRadioButton::toggled, this, [this]() { updateEnableTargetList(); });
    connect(ui->pushButton_target_import_folder, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this,                        // 親ウィジェット
                                                        tr("Select Folder"),         // ダイアログタイトル
                                                        "",                          // 初期ディレクトリ
                                                        QFileDialog::ShowDirsOnly    // ディレクトリのみ表示
        );
        if (!dir.isEmpty()) {
            if (!ui->radioButton_target_folder->isChecked()) {
                ui->radioButton_target_folder->setChecked(true);
            }

            QStringList files;

            QDirIterator it(dir, QDir::Files, QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                QString subFilePath = it.next();

                QString ext = QFileInfo(subFilePath).suffix().toLower();
                if (ext == "vox") {
                    files << subFilePath;
                }
                else if (ext == "opt") {
                    files << subFilePath;
                }
                else if (ext == "op2") {
                    files << subFilePath;
                }
                else if (ext == "fdtd") {
                    files << subFilePath;
                }
            }
            addTargetList(files);
        }
    });
    connect(ui->pushButton_target_open_folder, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this,                        // 親ウィジェット
                                                        tr("Select Folder"),         // ダイアログタイトル
                                                        "",                          // 初期ディレクトリ
                                                        QFileDialog::ShowDirsOnly    // ディレクトリのみ表示
        );
        if (!dir.isEmpty()) {
            if (!ui->radioButton_target_folder->isChecked()) {
                ui->radioButton_target_folder->setChecked(true);
            }

            targetListDelete(true);

            QStringList files;

            QDirIterator it(dir, QDir::Files, QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                QString subFilePath = it.next();

                QString ext = QFileInfo(subFilePath).suffix().toLower();
                if (ext == "vox") {
                    files << subFilePath;
                }
                else if (ext == "opt") {
                    files << subFilePath;
                }
                else if (ext == "op2") {
                    files << subFilePath;
                }
                else if (ext == "fdtd") {
                    files << subFilePath;
                }
            }
            addTargetList(files);
        }
    });
    connect(ui->pushButton_target_import_files, &QPushButton::clicked, this, [this]() {
        QString filter = tr("All Files (*.vox *.opt *.op2 *.fdtd);;vox file (*.vox);;opt file (*.opt);;op2 file "
                            "(*.op2);;fdtd file (*.fdtd)");

        QStringList loadfileNames = QFileDialog::getOpenFileNames(this, tr("Select file"), "", filter);

        if (!loadfileNames.isEmpty()) {
            if (!ui->radioButton_target_folder->isChecked()) {
                ui->radioButton_target_folder->setChecked(true);
            }

            addTargetList(loadfileNames);
        }
    });

    ui->radioButton_colormap_value_current->setChecked(true);
    ui->lineEdit_colormap_specify_min->setEnabled(false);
    ui->lineEdit_colormap_specify_max->setEnabled(false);
    connect(ui->radioButton_colormap_value_specify, &QRadioButton::toggled, this, [this]() {
        if (ui->radioButton_colormap_value_specify->isChecked()) {
            ui->lineEdit_colormap_specify_min->setEnabled(true);
            ui->lineEdit_colormap_specify_max->setEnabled(true);
        }
        else {
            ui->lineEdit_colormap_specify_min->setEnabled(false);
            ui->lineEdit_colormap_specify_max->setEnabled(false);
        }
    });

    ui->radioButton_output2d_output_target->setChecked(true);
    ui->lineEdit_output2d_output_folder->setEnabled(false);
    connect(ui->radioButton_output2d_output_target, &QRadioButton::toggled, this, [this]() {
        if (ui->radioButton_output2d_output_target->isChecked()) {
            ui->lineEdit_output2d_output_folder->setEnabled(false);
        }
        else {
            ui->lineEdit_output2d_output_folder->setEnabled(true);
        }
    });
    connect(ui->pushButton_output2d_output_folder, &QPushButton::clicked, this, [this]() {
        QString init_dir = ui->lineEdit_output2d_output_folder->text();
        QString dir      = QFileDialog::getExistingDirectory(this,                        // 親ウィジェット
                                                             tr("Select Folder"),         // ダイアログタイトル
                                                             init_dir,                    // 初期ディレクトリ
                                                             QFileDialog::ShowDirsOnly    // ディレクトリのみ表示
             );
        if (!dir.isEmpty()) {
            ui->radioButton_output2d_output_folder->setChecked(true);
            ui->lineEdit_output2d_output_folder->setEnabled(true);
            ui->lineEdit_output2d_output_folder->setText(dir);
        }
    });

    ui->radioButton_picture_size_data->setChecked(true);
    ui->doubleSpinBox_picture_size_data->setEnabled(true);
    ui->doubleSpinBox_picture_size_data->setValue(2.0);
    ui->spinBox_picture_size_specify_w->setEnabled(false);
    ui->spinBox_picture_size_specify_h->setEnabled(false);
    connect(ui->radioButton_picture_size_data, &QRadioButton::toggled, this, [this]() {
        if (ui->radioButton_picture_size_data->isChecked()) {
            ui->doubleSpinBox_picture_size_data->setEnabled(true);
            ui->spinBox_picture_size_specify_w->setEnabled(false);
            ui->spinBox_picture_size_specify_h->setEnabled(false);
        }
        else if (ui->radioButton_picture_size_specify->isChecked()) {
            ui->doubleSpinBox_picture_size_data->setEnabled(false);
            ui->spinBox_picture_size_specify_w->setEnabled(true);
            ui->spinBox_picture_size_specify_h->setEnabled(true);
        }
        else {
            ui->doubleSpinBox_picture_size_data->setEnabled(false);
            ui->spinBox_picture_size_specify_w->setEnabled(false);
            ui->spinBox_picture_size_specify_h->setEnabled(false);
        }
    });
    connect(ui->radioButton_picture_size_specify, &QRadioButton::toggled, this, [this]() {
        if (ui->radioButton_picture_size_data->isChecked()) {
            ui->doubleSpinBox_picture_size_data->setEnabled(true);
            ui->spinBox_picture_size_specify_w->setEnabled(false);
            ui->spinBox_picture_size_specify_h->setEnabled(false);
        }
        else if (ui->radioButton_picture_size_specify->isChecked()) {
            ui->doubleSpinBox_picture_size_data->setEnabled(false);
            ui->spinBox_picture_size_specify_w->setEnabled(true);
            ui->spinBox_picture_size_specify_h->setEnabled(true);
        }
        else {
            ui->doubleSpinBox_picture_size_data->setEnabled(false);
            ui->spinBox_picture_size_specify_w->setEnabled(false);
            ui->spinBox_picture_size_specify_h->setEnabled(false);
        }
    });

    /// Opt
    ui->cb_xy->setChecked(true);
    ui->cb_yz->setChecked(true);
    ui->cb_xz->setChecked(true);
    optSurfaceXYUpdate();
    optSurfaceYZUpdate();
    optSurfaceXZUpdate();
    connect(ui->cb_xy, &QCheckBox::toggled, this, [this]() { optSurfaceXYUpdate(); });
    connect(ui->cb_yz, &QCheckBox::toggled, this, [this]() { optSurfaceYZUpdate(); });
    connect(ui->cb_xz, &QCheckBox::toggled, this, [this]() { optSurfaceXZUpdate(); });
    connect(ui->pushButton_setminmax_xy, &QPushButton::clicked, this, [this]() { optSurfaceXYSetMinMax(); });
    connect(ui->pushButton_setminmax_yz, &QPushButton::clicked, this, [this]() { optSurfaceYZSetMinMax(); });
    connect(ui->pushButton_setminmax_xz, &QPushButton::clicked, this, [this]() { optSurfaceZXSetMinMax(); });
    // connect(ui->pushButton_clear_xy, &QPushButton::clicked, this, [this]() { optSurfaceXYClear(); });
    // connect(ui->pushButton_clear_yz, &QPushButton::clicked, this, [this]() { optSurfaceYZClear(); });
    // connect(ui->pushButton_clear_xz, &QPushButton::clicked, this, [this]() { optSurfaceZXClear(); });
    connect(ui->pushButton_setminmax_all, &QPushButton::clicked, this, [this]() { optSurfaceAll(); });
    connect(ui->le_xy, &QLineEdit::editingFinished, this, [this]() {
        ui->le_xy->setText(correctInputOptSurfaceValue(ui->le_xy->text(), ui->sp_min_xy->decimals()));
    });
    connect(ui->le_yz, &QLineEdit::editingFinished, this, [this]() {
        ui->le_yz->setText(correctInputOptSurfaceValue(ui->le_yz->text(), ui->sp_min_yz->decimals()));
    });
    connect(ui->le_xz, &QLineEdit::editingFinished, this, [this]() {
        ui->le_xz->setText(correctInputOptSurfaceValue(ui->le_xz->text(), ui->sp_min_xz->decimals()));
    });
    connect(ui->pb_add_xy, &QPushButton::clicked, this, [this]() { optSurfaceXYAdd(); });
    connect(ui->pb_add_yz, &QPushButton::clicked, this, [this]() { optSurfaceYZAdd(); });
    connect(ui->pb_add_xz, &QPushButton::clicked, this, [this]() { optSurfaceZXAdd(); });

    connect(ui->pushButton_output2d_picture, &QPushButton::clicked, this, [this]() { outputPictureText(); });

    ui->checkBox_2d_target_image->setChecked(false);
    ui->checkBox_2d_target_text->setChecked(false);
    bool enabled = ui->checkBox_2d_target_image->isChecked();
    ui->groupBox_colormap_value->setEnabled(enabled);
    ui->groupBox_picture_format->setEnabled(enabled);
    ui->groupBox_picture_size->setEnabled(enabled);
    ui->groupBox_image_other_setting->setEnabled(enabled);
    ui->pushButton_apply_default->setEnabled(enabled);
    ui->pushButton_set_default->setEnabled(enabled);
    ui->pushButton_init_default->setEnabled(enabled);
    ui->checkBox_change_background->setEnabled(enabled);
    ui->pushButton_background_color->setEnabled(enabled && ui->checkBox_change_background->isChecked());
    ui->checkBox_background_grad->setEnabled(enabled && ui->checkBox_change_background->isChecked());
    connect(ui->checkBox_2d_target_image, &QCheckBox::toggled, this, [this]() {
        bool enabled = ui->checkBox_2d_target_image->isChecked();
        ui->groupBox_colormap_value->setEnabled(enabled);
        ui->groupBox_picture_format->setEnabled(enabled);
        ui->groupBox_picture_size->setEnabled(enabled);
        ui->groupBox_image_other_setting->setEnabled(enabled);
        ui->pushButton_apply_default->setEnabled(enabled);
        ui->pushButton_set_default->setEnabled(enabled);
        ui->pushButton_init_default->setEnabled(enabled);
        ui->checkBox_change_background->setEnabled(enabled);
        ui->pushButton_background_color->setEnabled(enabled && ui->checkBox_change_background->isChecked());
        ui->checkBox_background_grad->setEnabled(enabled && ui->checkBox_change_background->isChecked());
    });
    connect(ui->checkBox_change_background, &QCheckBox::toggled, this, [this]() {
        bool enabled = ui->checkBox_2d_target_image->isChecked();
        ui->pushButton_background_color->setEnabled(enabled && ui->checkBox_change_background->isChecked());
        ui->checkBox_background_grad->setEnabled(enabled && ui->checkBox_change_background->isChecked());
    });

    connect(ui->pushButton_background_color, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_background_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_background_color->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            m_background_color = selectedColor;
        }
    });

    connect(ui->pushButton_apply_default, &QPushButton::clicked, this, [this]() { applyDefault(true); });
    connect(ui->pushButton_set_default, &QPushButton::clicked, this, [this]() { setDefault(); });
    connect(ui->pushButton_init_default, &QPushButton::clicked, this, [this]() { initDefault(); });

    ui->radioButton_opt_real_scale->setChecked(true);
    ui->radioButton_opt_cell_scale->setChecked(false);
    connect(ui->radioButton_opt_real_scale, &QRadioButton::toggled, this, [this]() { changeOptSurfaceScale(); });

    updateOptSurfaceScale();

    /// 1d
    ui->radioButton_1d_real_scale->setChecked(true);
    ui->radioButton_1d_cell_scale->setChecked(false);
    connect(ui->radioButton_1d_real_scale, &QRadioButton::toggled, this, [this]() { change1dDirectionScale(); });

    ui->checkBox_1d_target_text->setChecked(false);
    ui->groupBox_1d_text_direction->setEnabled(false);
    connect(ui->checkBox_1d_target_text, &QCheckBox::toggled, this, [this]() {
        bool enabled = ui->checkBox_1d_target_text->isChecked();
        ui->groupBox_1d_text_direction->setEnabled(enabled);
    });

    update1dDirectionScale();

    ui->checkBox_1d_xy_x->setChecked(true);
    ui->checkBox_1d_xy_y->setChecked(true);
    ui->checkBox_1d_yz_y->setChecked(true);
    ui->checkBox_1d_yz_z->setChecked(true);
    ui->checkBox_1d_xz_x->setChecked(true);
    ui->checkBox_1d_xz_z->setChecked(true);
    ui->checkBox_1d_dz_d->setChecked(true);
    ui->checkBox_1d_dz_z->setChecked(true);
    oneDirectionXY_XUpdate();
    oneDirectionXY_YUpdate();
    oneDirectionYZ_YUpdate();
    oneDirectionYZ_ZUpdate();
    oneDirectionXZ_XUpdate();
    oneDirectionXZ_ZUpdate();
    oneDirectionDZ_DUpdate();
    oneDirectionDZ_ZUpdate();
    connect(ui->checkBox_1d_xy_x, &QCheckBox::toggled, this, [this]() { oneDirectionXY_XUpdate(); });
    connect(ui->checkBox_1d_xy_y, &QCheckBox::toggled, this, [this]() { oneDirectionXY_YUpdate(); });
    connect(ui->checkBox_1d_yz_y, &QCheckBox::toggled, this, [this]() { oneDirectionYZ_YUpdate(); });
    connect(ui->checkBox_1d_yz_z, &QCheckBox::toggled, this, [this]() { oneDirectionYZ_ZUpdate(); });
    connect(ui->checkBox_1d_xz_x, &QCheckBox::toggled, this, [this]() { oneDirectionXZ_XUpdate(); });
    connect(ui->checkBox_1d_xz_z, &QCheckBox::toggled, this, [this]() { oneDirectionXZ_ZUpdate(); });
    connect(ui->checkBox_1d_dz_d, &QCheckBox::toggled, this, [this]() { oneDirectionDZ_DUpdate(); });
    connect(ui->checkBox_1d_dz_z, &QCheckBox::toggled, this, [this]() { oneDirectionDZ_ZUpdate(); });

    connect(ui->le_1d_xy_x, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_xy_x->setText(correctInputOptSurfaceValue(ui->le_1d_xy_x->text(), ui->sp_min_1d->decimals()));
    });
    connect(ui->le_1d_xy_y, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_xy_y->setText(correctInputOptSurfaceValue(ui->le_1d_xy_y->text(), ui->sp_min_1d->decimals()));
    });
    connect(ui->le_1d_yz_y, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_yz_y->setText(correctInputOptSurfaceValue(ui->le_1d_yz_y->text(), ui->sp_min_1d->decimals()));
    });
    connect(ui->le_1d_yz_z, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_yz_z->setText(correctInputOptSurfaceValue(ui->le_1d_yz_z->text(), ui->sp_min_1d->decimals()));
    });
    connect(ui->le_1d_xz_x, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_xz_x->setText(correctInputOptSurfaceValue(ui->le_1d_xz_x->text(), ui->sp_min_1d->decimals()));
    });
    connect(ui->le_1d_xz_z, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_xz_z->setText(correctInputOptSurfaceValue(ui->le_1d_xz_z->text(), ui->sp_min_1d->decimals()));
    });
    connect(ui->le_1d_dz_d, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_dz_d->setText(correctInputOptSurfaceValue(ui->le_1d_dz_d->text(), ui->sp_min_1d->decimals()));
    });
    connect(ui->le_1d_dz_z, &QLineEdit::editingFinished, this, [this]() {
        ui->le_1d_dz_z->setText(correctInputOptSurfaceValue(ui->le_1d_dz_z->text(), ui->sp_min_1d->decimals()));
    });

    connect(ui->pb_add_1d_xy_x, &QPushButton::clicked, this, [this]() { oneDirectionXY_XAdd(); });
    connect(ui->pb_add_1d_xy_y, &QPushButton::clicked, this, [this]() { oneDirectionXY_YAdd(); });
    connect(ui->pb_add_1d_yz_y, &QPushButton::clicked, this, [this]() { oneDirectionYZ_YAdd(); });
    connect(ui->pb_add_1d_yz_z, &QPushButton::clicked, this, [this]() { oneDirectionYZ_ZAdd(); });
    connect(ui->pb_add_1d_xz_x, &QPushButton::clicked, this, [this]() { oneDirectionXZ_XAdd(); });
    connect(ui->pb_add_1d_xz_z, &QPushButton::clicked, this, [this]() { oneDirectionXZ_ZAdd(); });
    connect(ui->pb_add_1d_dz_d, &QPushButton::clicked, this, [this]() { oneDirectionDZ_DAdd(); });
    connect(ui->pb_add_1d_dz_z, &QPushButton::clicked, this, [this]() { oneDirectionDZ_ZAdd(); });

    updateEnableTargetList();
    createTargetList();

    ui->lineEdit_output2d_output_folder->setDragDropFoloder();

    applyDefault();

    adjustSize();

    updateDlg();
}

ResultOutputDlg::~ResultOutputDlg()
{
    delete m_file_explorer_dlg;
    delete ui;
}

void ResultOutputDlg::optSurfaceXYUpdate()
{
    if (ui->cb_xy->isChecked()) {
        // ui->sp_min_xy->setEnabled(true);
        // ui->sp_max_xy->setEnabled(true);
        // ui->sp_step_xy->setEnabled(true);
        // ui->pb_add_xy->setEnabled(true);
        ui->le_xy->setEnabled(true);
        // ui->pushButton_setminmax_xy->setEnabled(true);
        // ui->pushButton_clear_xy->setEnabled(true);
    }
    else {
        // ui->sp_min_xy->setEnabled(false);
        // ui->sp_max_xy->setEnabled(false);
        // ui->sp_step_xy->setEnabled(false);
        // ui->pb_add_xy->setEnabled(false);
        ui->le_xy->setEnabled(false);
        // ui->pushButton_setminmax_xy->setEnabled(false);
        // ui->pushButton_clear_xy->setEnabled(false);
    }
}

void ResultOutputDlg::optSurfaceYZUpdate()
{
    if (ui->cb_yz->isChecked()) {
        // ui->sp_min_yz->setEnabled(true);
        // ui->sp_max_yz->setEnabled(true);
        // ui->sp_step_yz->setEnabled(true);
        // ui->pb_add_yz->setEnabled(true);
        ui->le_yz->setEnabled(true);
        // ui->pushButton_setminmax_yz->setEnabled(true);
        // ui->pushButton_clear_yz->setEnabled(true);
    }
    else {
        // ui->sp_min_yz->setEnabled(false);
        // ui->sp_max_yz->setEnabled(false);
        // ui->sp_step_yz->setEnabled(false);
        // ui->pb_add_yz->setEnabled(false);
        ui->le_yz->setEnabled(false);
        // ui->pushButton_setminmax_yz->setEnabled(false);
        // ui->pushButton_clear_yz->setEnabled(false);
    }
}

void ResultOutputDlg::optSurfaceXZUpdate()
{
    if (ui->cb_xz->isChecked()) {
        // ui->sp_min_xz->setEnabled(true);
        // ui->sp_max_xz->setEnabled(true);
        // ui->sp_step_xz->setEnabled(true);
        // ui->pb_add_xz->setEnabled(true);
        ui->le_xz->setEnabled(true);
        // ui->pushButton_setminmax_xz->setEnabled(true);
        // ui->pushButton_clear_xz->setEnabled(true);
    }
    else {
        // ui->sp_min_xz->setEnabled(false);
        // ui->sp_max_xz->setEnabled(false);
        // ui->sp_step_xz->setEnabled(false);
        // ui->pb_add_xz->setEnabled(false);
        ui->le_xz->setEnabled(false);
        // ui->pushButton_setminmax_xz->setEnabled(false);
        // ui->pushButton_clear_xz->setEnabled(false);
    }
}

void ResultOutputDlg::optSurfaceXYSetMinMax()
{
    auto voxel = m_3DForm->resultCtl()->opt3D();
    if (voxel) {
        VoxelScalar* voxel_scalar = voxel->object<VoxelScalar>();
        if (voxel_scalar) {
            ui->sp_min_xy->setValue(0);
            if (ui->radioButton_opt_real_scale->isChecked()) {
                ui->sp_max_xy->setValue(((float)voxel_scalar->originalZ() - 1.0f) * voxel_scalar->originalDZ()
                                        * 1.0e3f);
            }
            else {
                ui->sp_max_xy->setValue(voxel_scalar->originalZ() - 1);
            }
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているOptのMinMaxを設定します。\nOptが読込まれてないため設定できません。");
    }
}

void ResultOutputDlg::optSurfaceYZSetMinMax()
{
    auto voxel = m_3DForm->resultCtl()->opt3D();
    if (voxel) {
        VoxelScalar* voxel_scalar = voxel->object<VoxelScalar>();
        if (voxel_scalar) {
            ui->sp_min_yz->setValue(0);
            if (ui->radioButton_opt_real_scale->isChecked()) {
                ui->sp_max_yz->setValue(((float)voxel_scalar->originalX() - 1.0f) * voxel_scalar->originalDX()
                                        * 1.0e3f);
            }
            else {
                ui->sp_max_yz->setValue(voxel_scalar->originalX() - 1);
            }
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているOptのMinMaxを設定します。\nOptが読込まれてないため設定できません。");
    }
}
void ResultOutputDlg::optSurfaceZXSetMinMax()
{
    auto voxel = m_3DForm->resultCtl()->opt3D();
    if (voxel) {
        VoxelScalar* voxel_scalar = voxel->object<VoxelScalar>();
        if (voxel_scalar) {
            ui->sp_min_xz->setValue(0);
            if (ui->radioButton_opt_real_scale->isChecked()) {
                ui->sp_max_xz->setValue(((float)voxel_scalar->originalY() - 1.0f) * voxel_scalar->originalDY()
                                        * 1.0e3f);
            }
            else {
                ui->sp_max_xz->setValue(voxel_scalar->originalY() - 1);
            }
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているOptのMinMaxを設定します。\nOptが読込まれてないため設定できません。");
    }
}

void ResultOutputDlg::optSurfaceXYClear()
{
    ui->le_xy->setText("");
}

void ResultOutputDlg::optSurfaceYZClear()
{
    ui->le_yz->setText("");
}

void ResultOutputDlg::optSurfaceZXClear()
{
    ui->le_xz->setText("");
}

void ResultOutputDlg::optSurfaceAll()
{
    auto voxel = m_3DForm->resultCtl()->opt3D();
    if (voxel) {
        VoxelScalar* voxel_scalar = voxel->object<VoxelScalar>();
        if (voxel_scalar) {
            ui->sp_min_xz->setValue(0);

            ui->sp_min_yz->setValue(0);

            ui->sp_min_xy->setValue(0);

            if (ui->radioButton_opt_real_scale->isChecked()) {
                ui->sp_max_xy->setValue(((float)voxel_scalar->originalZ() - 1.0f) * voxel_scalar->originalDZ()
                                        * 1.0e3f);
                ui->sp_max_yz->setValue(((float)voxel_scalar->originalX() - 1.0f) * voxel_scalar->originalDX()
                                        * 1.0e3f);
                ui->sp_max_xz->setValue(((float)voxel_scalar->originalY() - 1.0f) * voxel_scalar->originalDY()
                                        * 1.0e3f);

                ui->sp_step_xy->setValue((float)voxel_scalar->originalDZ() * 1.0e3f);
                ui->sp_step_yz->setValue((float)voxel_scalar->originalDX() * 1.0e3f);
                ui->sp_step_xz->setValue((float)voxel_scalar->originalDY() * 1.0e3f);
            }
            else {
                ui->sp_max_xz->setValue(voxel_scalar->originalY() - 1);
                ui->sp_max_yz->setValue(voxel_scalar->originalX() - 1);
                ui->sp_max_xy->setValue(voxel_scalar->originalZ() - 1);

                ui->sp_step_xy->setValue(1);
                ui->sp_step_yz->setValue(1);
                ui->sp_step_xz->setValue(1);
            }

            optSurfaceXYClear();
            optSurfaceYZClear();
            optSurfaceZXClear();

            optSurfaceXYAdd();
            optSurfaceYZAdd();
            optSurfaceZXAdd();
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているOpt面を設定します。\nOptが読込まれてないため設定できません。");
    }
}

QString ResultOutputDlg::summarizeValues(const std::set<float>& values, int /* decimals*/)
{
    if (values.empty()) return "";

    /// 個数少なければそのまま
    if (values.size() < 20) {
        QString result;
        for (auto& value : values) {
            int decimals = valueDecimals(value);
            result += QString::number(value, 'f', decimals) + ",";
        }
        // 末尾のカンマ除去
        if (!result.isEmpty()) result.chop(1);
        return result;
    }

    QString result;
    auto    itr = values.begin();
    while (itr != values.end()) {
        float start = *itr;
        auto  next  = std::next(itr);
        if (next == values.end()) {
            // 最後の値
            int decimals = valueDecimals(start);
            result += QString::number(start, 'f', decimals) + ",";
            break;
        }

        float step = *next - start;
        // ステップが0の場合は個別値
        if (step == 0) {
            int decimals = valueDecimals(start);
            result += QString::number(start, 'f', decimals) + ",";
            itr = next;
            continue;
        }

        // 等差数列を検出
        auto end_itr = next;
        while (std::next(end_itr) != values.end() && qFuzzyCompare(*std::next(end_itr) - *end_itr, step)) {
            end_itr++;
        }

        int count = std::distance(itr, end_itr) + 1;
        if (count >= 3) {
            // 3つ以上なら範囲＋ステップでまとめる
            float end = *end_itr;

            int start_decimals = valueDecimals(start);
            int end_decimals   = valueDecimals(end);
            int step_decimals  = valueDecimals(step);

            result += QString("%1-%2:%3,")
                          .arg(QString::number(start, 'f', start_decimals))
                          .arg(QString::number(end, 'f', end_decimals))
                          .arg(QString::number(step, 'f', step_decimals));
            itr = std::next(end_itr);
        }
        else {
            int decimals = valueDecimals(start);
            // 2つ以下なら個別値
            result += QString::number(start, 'f', decimals) + ",";
            itr = next;
        }
    }

    // 末尾のカンマ除去
    if (!result.isEmpty()) result.chop(1);
    return result;
}

std::set<float> ResultOutputDlg::strToSurfaceValue(QString str, int decimals)
{
    std::set<float> valid_values;

    // スペースをカンマに変換
    str.replace(" ", ",");

    // カンマで分割
    const auto& values_str = str.split(",");

    for (const auto& value : values_str) {
        QString v = value.trimmed();
        // 範囲＋ステップ表記か判定
        static QRegularExpression re("^(\\d+(?:\\.\\d+)?)-(\\d+(?:\\.\\d+)?):(\\d+(?:\\.\\d+)?)$");
        QRegularExpressionMatch   match = re.match(v);
        if (match.hasMatch()) {
            float start = match.captured(1).toFloat();
            float end   = match.captured(2).toFloat();
            float step  = match.captured(3).toFloat();
            if (step > 0) {
                for (float n = start; n <= end; n += step) {
                    valid_values.insert(n);
                }
            }
        }
        else {
            // 通常の数値
            bool  valid  = false;
            float number = v.toFloat(&valid);
            if (valid && number >= 0) {
                valid_values.insert(number);
            }
        }
    }

    return valid_values;
}

QString ResultOutputDlg::correctInputOptSurfaceValue(QString str, int decimals)
{
    return summarizeValues(strToSurfaceValue(str, decimals), decimals);
}

int ResultOutputDlg::valueDecimals(float value)
{
    int decimals = 0;
    if (value != static_cast<int>(value)) {
        QString stepStr  = QString::number(value, 'f', 6);
        int     dotIndex = stepStr.indexOf('.');
        if (dotIndex != -1) {
            decimals = stepStr.length() - dotIndex - 1;
            while (decimals > 0 && stepStr.endsWith('0')) {
                stepStr.chop(1);
                decimals--;
            }
        }
    }
    return decimals;
}

void ResultOutputDlg::updateEnableTargetList()
{
    bool enable_list = false;

    if (ui->radioButton_target_current->isChecked()) {
    }
    else if (ui->radioButton_target_current_disp->isChecked()) {
    }
    else {
        enable_list = true;
    }

    ui->tableWidget_target_list->setEnabled(enable_list);
    ui->label_target_list_array_x->setEnabled(enable_list);
    ui->label_target_list_array_y->setEnabled(enable_list);
    ui->spinBox_target_list_array_x->setEnabled(enable_list);
    ui->spinBox_target_list_array_y->setEnabled(enable_list);
    ui->pushButton_target_list_all_on->setEnabled(enable_list);
    ui->pushButton_target_list_all_off->setEnabled(enable_list);
    ui->pushButton_target_list_array_apply->setEnabled(enable_list);
    ui->pushButton_target_list_delete->setEnabled(enable_list);
    ui->pushButton_target_list_all_delete->setEnabled(enable_list);
}

void ResultOutputDlg::hideFileExplorerDlg()
{
    if (m_file_explorer_dlg && m_file_explorer_dlg->isVisible()) {
        m_file_explorer_dlg->hide();
    }
}

void ResultOutputDlg::createTargetList()
{
    QStringList labels;
    labels << ""
           << "File Name"
           << "Type"
           << "Folder"
           << "Array X"
           << "Array Y"
           << "Related File";

    ui->tableWidget_target_list->setColumnCount(labels.size());
    // ui->tableWidget_target_list->setStyleSheet("QTableWidget::item { padding: 0px; }");
    ui->tableWidget_target_list->setHorizontalHeaderLabels(labels);
    // ui->tableWidget_create_auto_dimension->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QVector<int> columnMargins = {10, 150, 15, 150, 10, 10, 200};    // 各列に対する余白の設定

    QFontMetrics font_metrics(ui->tableWidget_target_list->horizontalHeader()->font());
    for (int i = 0; i < labels.size(); ++i) {
        int size = font_metrics.horizontalAdvance(labels[i]);

        ui->tableWidget_target_list->setColumnWidth(i, size + columnMargins[i]);
    }

    // カスタムコンテキストメニューを有効化
    ui->tableWidget_target_list->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->pushButton_target_list_all_on, &QPushButton::clicked, this, [this]() { targetListCheck(true, true); });
    connect(ui->pushButton_target_list_all_off, &QPushButton::clicked, this,
            [this]() { targetListCheck(false, true); });
    connect(ui->pushButton_target_list_delete, &QPushButton::clicked, this, [this]() { targetListDelete(false); });
    connect(ui->pushButton_target_list_all_delete, &QPushButton::clicked, this, [this]() { targetListDelete(true); });

    connect(ui->pushButton_target_list_array_apply, &QPushButton::clicked, this,
            [this]() { targetListArrayApply(true, false); });

    // シグナルとスロットを接続
    QObject::connect(ui->tableWidget_target_list, &QTableWidget::customContextMenuRequested, [this](const QPoint& pos) {
        /// メニュー作成
        QMenu    contextMenu;
        QAction* action_show = contextMenu.addAction("On");
        QAction* action_hide = contextMenu.addAction("Off");
        contextMenu.addSeparator();
        QAction* action_array = contextMenu.addAction("Array Setting");
        contextMenu.addSeparator();
        QAction* action_releated_file_vox  = contextMenu.addAction("Related File op2/opt - vox");
        QAction* action_releated_file_fdtd = contextMenu.addAction("Related File vox - fdtd");
        contextMenu.addSeparator();
        QAction* action_del = contextMenu.addAction("Delete");

        /// 実行
        QAction* selectedAction = contextMenu.exec(ui->tableWidget_target_list->viewport()->mapToGlobal(pos));

        if (selectedAction == action_show) {
            targetListCheck(true, false);
        }
        else if (selectedAction == action_hide) {
            targetListCheck(false, false);
        }
        else if (selectedAction == action_array) {
            targetListArrayApply(false, true);
        }
        else if (selectedAction == action_del) {
            targetListDelete(false);
        }
        else if (selectedAction == action_releated_file_vox) {
            targetListRelatedFileOpVox();
        }
        else if (selectedAction == action_releated_file_fdtd) {
            targetListRelatedFileVoxFdtd();
        }
    });

    connect(ui->tableWidget_target_list->horizontalHeader(), &QHeaderView::sectionClicked, this,
            [this](int) { ui->tableWidget_target_list->setSortingEnabled(true); });

    ui->tableWidget_target_list->setAcceptDrops(true);

    ui->tableWidget_target_list->setDropCallback([this](const QStringList& paths) {
        QStringList file_paths;
        for (const QString& filePath : paths) {
            QFileInfo info(filePath);

            if (info.isDir()) {
                QDirIterator it(filePath, QDir::Files, QDirIterator::NoIteratorFlags);
                while (it.hasNext()) {
                    QString subFilePath = it.next();

                    QString ext = QFileInfo(subFilePath).suffix().toLower();
                    if (ext == "vox") {
                        file_paths << subFilePath;
                    }
                    else if (ext == "opt") {
                        file_paths << subFilePath;
                    }
                    else if (ext == "op2") {
                        file_paths << subFilePath;
                    }
                    else if (ext == "fdtd") {
                        file_paths << subFilePath;
                    }
                }
            }
            else {
                QString ext = QFileInfo(filePath).suffix().toLower();
                if (ext == "vox") {
                    file_paths << filePath;
                }
                else if (ext == "opt") {
                    file_paths << filePath;
                }
                else if (ext == "op2") {
                    file_paths << filePath;
                }
                else if (ext == "fdtd") {
                    file_paths << filePath;
                }
            }
        }

        addTargetList(file_paths);
    });

    ui->tableWidget_target_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_target_list->setItemDelegate(new NoElideDelegate(ui->tableWidget_target_list));
}

void ResultOutputDlg::addTargetList(const QStringList& paths)
{
    QStringList vox_files;
    QStringList fdtd_files;
    QStringList op2_files;
    QStringList opt_files;

    for (const auto& path : paths) {
        QString ext = QFileInfo(path).suffix().toLower();
        if (ext == "vox") {
            vox_files << path;
        }
        else if (ext == "opt") {
            opt_files << path;
        }
        else if (ext == "op2") {
            op2_files << path;
        }
        else if (ext == "fdtd") {
            fdtd_files << path;
        }
    }

    int x_array = ui->spinBox_target_list_array_x->value();
    int y_array = ui->spinBox_target_list_array_y->value();
    ReadOp2::maxArrayXY(op2_files, x_array, y_array, nullptr);

    ui->tableWidget_target_list->setUpdatesEnabled(false);
    ui->tableWidget_target_list->setSortingEnabled(false);

    int row = ui->tableWidget_target_list->rowCount();
    ui->tableWidget_target_list->setRowCount(row + op2_files.size() + opt_files.size() + vox_files.size()
                                             + fdtd_files.size());

    for (auto& op2 : op2_files) {
        if (addTargetList(op2, x_array, y_array, row)) {
            ++row;
        }
    }
    for (auto& opt : opt_files) {
        if (addTargetList(opt, 0, 0, row)) {
            ++row;
        }
    }
    for (auto& vox : vox_files) {
        if (addTargetList(vox, 0, 0, row)) {
            ++row;
        }
    }
    for (auto& fdtd : fdtd_files) {
        if (addTargetList(fdtd, 0, 0, row)) {
            ++row;
        }
    }
    ui->tableWidget_target_list->setRowCount(row);

    ui->tableWidget_target_list->setSortingEnabled(true);
    ui->tableWidget_target_list->setUpdatesEnabled(true);

    ui->spinBox_target_list_array_x->setValue(x_array);
    ui->spinBox_target_list_array_y->setValue(y_array);

    /// Op2/Opt関連付け
    /// Vox関連付け
    setTargetListRelatedFile();
}

bool ResultOutputDlg::addTargetList(const QString& path, int x_array, int y_array, int row)
{
    QString path_lower = path;
    if (m_target_all_files.insert(path_lower.toLower()).second == false) {
        return false;
    }

    QFileInfo file_info(path);

    QString ext = file_info.suffix().toLower();

    auto table_widget = ui->tableWidget_target_list;

    table_widget->setRowHeight(row, 13);

    int column = 0;

    /// Target
    auto checkBox_text = new QTableWidgetItem();    /// ソート用
    table_widget->setItem(row, column, checkBox_text);
    QWidget*     checkBoxWidget = new QWidget();
    QCheckBox*   checkBox       = new QCheckBox(checkBoxWidget);
    QHBoxLayout* layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);

    if (ext == "op2" || ext == "opt") {
        checkBox->setChecked(true);
        checkBox_text->setText("");
    }
    else {
        /// 対象ではないので
        checkBox->setChecked(false);
        checkBox->setVisible(false);
        checkBox_text->setText("  ");
    }
    connect(checkBox, &QCheckBox::toggled, this, [this, checkBox_text](bool checked) {
        if (checked) {
            ui->tableWidget_target_list->setSortingEnabled(false);
            checkBox_text->setText("");
        }
        else {
            ui->tableWidget_target_list->setSortingEnabled(false);
            checkBox_text->setText(" ");
        }
    });

    /// File
    QTableWidgetItem* file_item = new QTableWidgetItem(file_info.completeBaseName());
    file_item->setFlags(file_item->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, file_item);
    file_item->setToolTip(file_item->text());
    file_item->setData(Qt::UserRole, path);    /// フルパスを保持

    /// Type
    QTableWidgetItem* type_item = new QTableWidgetItem(ext);
    type_item->setFlags(type_item->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, type_item);

    /// Folder
    QTableWidgetItem* folder_item = new QTableWidgetItem(file_info.absoluteDir().absolutePath());
    folder_item->setFlags(folder_item->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, folder_item);
    folder_item->setToolTip(file_info.absoluteDir().absolutePath());

    /// "Array X"
    bool array_xy = false;
    if (ext == "op2") {
        QString     symbol, symbol2, number, number2;
        const auto& file_name = QFileInfo(path).completeBaseName();
        if (ResultCtrl::sectionName(file_name, symbol, number, symbol2, number2)) {
            if (symbol.compare("D", Qt::CaseInsensitive) == 0) {
                array_xy = true;
            }
        }
    }
    if (array_xy) {
        CustomSpin* cell_x = new CustomSpin();
        cell_x->setRange(1, 999);
        cell_x->setValue(x_array);
        cell_x->setContextMenuPolicy(Qt::NoContextMenu);
        auto text_item = new QTableWidgetItem();
        text_item->setText(QString::number(cell_x->value()).rightJustified(5, '0'));
        /// Dummyでソートできるようにするため
        connect(cell_x, &QSpinBox::valueChanged, this, [this, cell_x, text_item](int) {
            ui->tableWidget_target_list->setSortingEnabled(
                false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
            text_item->setText(QString::number(cell_x->value()).rightJustified(5, '0'));
        });
        table_widget->setItem(row, column, text_item);
        table_widget->setCellWidget(row, column++, cell_x);

        CustomSpin* cell_y = new CustomSpin();
        cell_y->setRange(1, 999);
        cell_y->setValue(y_array);
        cell_y->setContextMenuPolicy(Qt::NoContextMenu);
        text_item = new QTableWidgetItem();
        text_item->setText(QString::number(cell_y->value()).rightJustified(5, '0'));
        /// Dummyでソートできるようにするため
        connect(cell_y, &QSpinBox::valueChanged, this, [this, cell_y, text_item](int) {
            ui->tableWidget_target_list->setSortingEnabled(
                false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
            text_item->setText(QString::number(cell_y->value()).rightJustified(5, '0'));
        });
        table_widget->setItem(row, column, text_item);
        table_widget->setCellWidget(row, column++, cell_y);
    }
    else {
        auto text_item = new QTableWidgetItem();
        /// Dummyでソートできるようにするため
        text_item->setText("");
        table_widget->setItem(row, column, text_item);
        column++;

        text_item = new QTableWidgetItem();
        /// Dummyでソートできるようにするため
        text_item->setText("");
        table_widget->setItem(row, column, text_item);
        column++;
    }

    /// Related File
    QTableWidgetItem* related_file_item = new QTableWidgetItem("");
    related_file_item->setFlags(related_file_item->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, related_file_item);

    return true;
}

void ResultOutputDlg::setTargetListRelatedFile()
{
    /// VoxFiles / FdtdFiles
    std::map<QString, std::vector<ResultCtrl::VoxNameData>>  key_vox_name_data;
    std::map<QString, std::vector<ResultCtrl::FdtdNameData>> key_fdtd_name_data;

    QStringList all_voxes;
    QStringList all_fdtd;

    int row_count = ui->tableWidget_target_list->rowCount();
    for (int row = 0; row < row_count; ++row) {
        const QString& ext  = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_TYPE)->text();
        const QString& path = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_FILE_FULLPATH)
                                  ->data(Qt::UserRole)
                                  .toString();

        if (ext == "vox") {
            const QString& vox_name = QFileInfo(path).completeBaseName();

            ResultCtrl::VoxNameData data;
            data.m_vox_names  = vox_name.split("_");
            data.m_vox_path   = path;
            data.m_vox_folder = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_FOLDER)->text();

            if (data.m_vox_names.size() > 0) {
                QString key = data.m_vox_names[0];
                key_vox_name_data[key.toLower()].emplace_back(data);
            }

            all_voxes << path;
        }
        else if (ext == "fdtd") {
            ResultCtrl::FdtdNameData data;
            data.m_path   = path;
            data.m_folder = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_FOLDER)->text();

            QString key = QFileInfo(path).completeBaseName().toLower();
            key_fdtd_name_data[key].emplace_back(data);

            all_fdtd << path;
        }
    }

    for (int row = 0; row < row_count; ++row) {
        const QString& ext  = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_TYPE)->text();
        const QString& path = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_FILE_FULLPATH)
                                  ->data(Qt::UserRole)
                                  .toString();

        if (ext == "vox") {
            auto cell_widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE);
            if (cell_widget) {
                QComboBox* combo_box    = (QComboBox*)cell_widget;
                QString    current_text = combo_box->currentText();

                bool valid_current = false;
                combo_box->clear();
                combo_box->addItem("(None)");    /// 未設定が必要
                for (auto& fdtd : all_fdtd) {
                    combo_box->addItem(fdtd);

                    if (!valid_current) {
                        if (current_text == fdtd) {
                            valid_current = true;
                        }
                    }
                }
                if (valid_current) {
                    combo_box->setCurrentText(current_text);
                }
                else {
                    if (all_fdtd.size() > 0) {
                        const QString& file_name    = QFileInfo(path).completeBaseName();
                        QString        related_file = ResultCtrl::relatedVoxFdtd(file_name, key_fdtd_name_data);
                        if (!related_file.isEmpty()) {
                            combo_box->setCurrentText(related_file);
                        }
                        else {
                            combo_box->setCurrentText("(None)");
                        }
                    }
                    else {
                        combo_box->setCurrentText("(None)");
                    }
                }
            }
            else {
                CustomCombo* combo_box = new CustomCombo();
                combo_box->addItem("(None)");    /// 未設定が必要
                for (auto& fdtd : all_fdtd) {
                    combo_box->addItem(fdtd);
                }

                auto table_item = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE);
                connect(combo_box, &QComboBox::currentTextChanged, this, [this, combo_box, table_item](const QString&) {
                    ui->tableWidget_target_list->setSortingEnabled(
                        false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
                    table_item->setText(combo_box->currentText());
                });

                if (all_fdtd.size() > 0) {
                    const QString& file_name    = QFileInfo(path).completeBaseName();
                    QString        related_file = ResultCtrl::relatedVoxFdtd(file_name, key_fdtd_name_data);
                    if (!related_file.isEmpty()) {
                        combo_box->setCurrentText(related_file);
                    }
                    else {
                        combo_box->setCurrentText("(None)");
                    }
                }
                else {
                    combo_box->setCurrentText("(None)");
                }

                ui->tableWidget_target_list->setCellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE, combo_box);
            }
        }
        else if (ext == "op2" || ext == "opt") {
            auto cell_widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE);
            if (cell_widget) {
                QComboBox* combo_box    = (QComboBox*)cell_widget;
                QString    current_text = combo_box->currentText();

                bool valid_current = false;
                combo_box->clear();
                combo_box->addItem("(None)");    /// 未設定が必要
                for (auto& vox : all_voxes) {
                    combo_box->addItem(vox);

                    if (!valid_current) {
                        if (current_text == vox) {
                            valid_current = true;
                        }
                    }
                }
                if (valid_current) {
                    combo_box->setCurrentText(current_text);
                }
                else {
                    if (all_voxes.size() > 0) {
                        const QString& file_name    = QFileInfo(path).completeBaseName();
                        QString        related_file = ResultCtrl::relatedVoxOp(file_name, key_vox_name_data);
                        if (!related_file.isEmpty()) {
                            combo_box->setCurrentText(related_file);
                        }
                        else {
                            combo_box->setCurrentText("(None)");
                        }
                    }
                    else {
                        combo_box->setCurrentText("(None)");
                    }
                }
            }
            else {
                CustomCombo* combo_box = new CustomCombo();
                combo_box->addItem("(None)");    /// 未設定が必要
                for (auto& vox : all_voxes) {
                    combo_box->addItem(vox);
                }

                auto table_item = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE);
                connect(combo_box, &QComboBox::currentTextChanged, this, [this, combo_box, table_item](const QString&) {
                    ui->tableWidget_target_list->setSortingEnabled(
                        false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
                    table_item->setText(combo_box->currentText());
                });

                if (all_voxes.size() > 0) {
                    const QString& file_name    = QFileInfo(path).completeBaseName();
                    QString        related_file = ResultCtrl::relatedVoxOp(file_name, key_vox_name_data);
                    if (!related_file.isEmpty()) {
                        combo_box->setCurrentText(related_file);
                    }
                    else {
                        combo_box->setCurrentText("(None)");
                    }
                }
                else {
                    combo_box->setCurrentText("(None)");
                }

                ui->tableWidget_target_list->setCellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE, combo_box);
            }
        }
    }
}

void ResultOutputDlg::targetListCheck(bool check, bool all)
{
    /// listにためる必要ないが他と合わせておく

    std::vector<QCheckBox*>        buttons;
    std::vector<QTableWidgetItem*> button_items;
    QItemSelectionModel*           selection_model = ui->tableWidget_target_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_target_list->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            auto cell_widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_TARGET);
            if (cell_widget) {
                auto check_box = cell_widget->findChild<QCheckBox*>();
                if (check_box->isVisible()) {
                    buttons.emplace_back(check_box);
                    button_items.emplace_back(ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_TARGET));
                }
            }
        }
    }

    ui->tableWidget_target_list->setSortingEnabled(false);

    for (int ic = 0; ic < buttons.size(); ++ic) {
        auto& button      = buttons[ic];
        auto& button_item = button_items[ic];

        button->blockSignals(true);
        button->setChecked(check);
        button_item->setText(check ? "" : " ");
        button->blockSignals(false);
    }
}

void ResultOutputDlg::targetListDelete(bool all)
{
    if (all) {
        m_target_all_files.clear();
    }

    QItemSelectionModel* selection_model = ui->tableWidget_target_list->selectionModel();
    for (int row = ui->tableWidget_target_list->rowCount() - 1; row >= 0; row--) {
        if (all || selection_model->isRowSelected(row)) {
            if (!all) {
                const QString& path = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_FILE_FULLPATH)
                                          ->data(Qt::UserRole)
                                          .toString();
                m_target_all_files.erase(path.toLower());
            }
            ui->tableWidget_target_list->removeRow(row);
        }
    }

    if (!all) {
        setTargetListRelatedFile();
    }
}

void ResultOutputDlg::targetListArrayApply(bool all, bool show_setting_dlg)
{
    /// Target
    std::set<int> targets;

    if (all) {
        for (int ic = 0; ic < ui->tableWidget_target_list->rowCount(); ++ic) {
            QWidget* widget = ui->tableWidget_target_list->cellWidget(ic, RESULT_OUTPUT_TARGET_LIST_ARRAY_X);
            if (widget) {
                targets.insert(ic);
            }
        }
    }
    else {
        QItemSelectionModel* selection_model = ui->tableWidget_target_list->selectionModel();
        QModelIndexList      selectedIndexes = selection_model->selectedRows();
        for (const QModelIndex& index : selectedIndexes) {
            QWidget* widget = ui->tableWidget_target_list->cellWidget(index.row(), RESULT_OUTPUT_TARGET_LIST_ARRAY_X);
            if (widget) {
                targets.insert(index.row());
            }
        }
    }

    if (targets.size() == 0) {
        return;
    }

    int arry_x = ui->spinBox_target_list_array_x->value();
    int arry_y = ui->spinBox_target_list_array_y->value();

    if (show_setting_dlg) {
        Op2ArraySettingDlg dlg(m_3DForm);

        if (targets.size() == 1) {
            int index = *targets.begin();
            arry_x =
                ((QSpinBox*)ui->tableWidget_target_list->cellWidget(index, RESULT_OUTPUT_TARGET_LIST_ARRAY_X))->value();
            arry_y =
                ((QSpinBox*)ui->tableWidget_target_list->cellWidget(index, RESULT_OUTPUT_TARGET_LIST_ARRAY_Y))->value();
        }

        dlg.setArrayX(arry_x);
        dlg.setArrayY(arry_y);
        dlg.hideVox();

        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        arry_x = dlg.arrayX();
        arry_y = dlg.arrayY();
    }

    ui->tableWidget_target_list->setSortingEnabled(false);

    /// Setting
    for (auto target : targets) {
        ((QSpinBox*)ui->tableWidget_target_list->cellWidget(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_X))
            ->blockSignals(true);
        ((QSpinBox*)ui->tableWidget_target_list->cellWidget(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_Y))
            ->blockSignals(true);

        ((QSpinBox*)ui->tableWidget_target_list->cellWidget(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_X))
            ->setValue(arry_x);
        ((QSpinBox*)ui->tableWidget_target_list->cellWidget(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_Y))
            ->setValue(arry_y);
        ui->tableWidget_target_list->item(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_X)
            ->setText(QString::number(arry_x).rightJustified(5, '0'));
        ui->tableWidget_target_list->item(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_Y)
            ->setText(QString::number(arry_y).rightJustified(5, '0'));

        ((QSpinBox*)ui->tableWidget_target_list->cellWidget(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_X))
            ->blockSignals(false);
        ((QSpinBox*)ui->tableWidget_target_list->cellWidget(target, RESULT_OUTPUT_TARGET_LIST_ARRAY_Y))
            ->blockSignals(false);
    }
}

void ResultOutputDlg::targetListRelatedFileOpVox()
{
    std::vector<QComboBox*>        buttons;
    std::vector<QTableWidgetItem*> button_items;
    QItemSelectionModel*           selection_model = ui->tableWidget_target_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_target_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            const QString& ext = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_TYPE)->text();

            if (ext == "op2" || ext == "opt") {
                buttons.emplace_back(
                    (QComboBox*)ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE));
                button_items.emplace_back(
                    ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE));
            }
        }
    }

    if (buttons.size() == 0) {
        return;
    }

    QString current_text = buttons[0]->currentText();
    /*
    for (int ic = 1; ic < buttons.size(); ++ic) {
        if (current_text != buttons[ic]->currentText()) {
            current_text = "(None)";
            break;
        }
    }*/

    ComboBoxDialog dlg("Related File op2/opt - vox", this);
    for (int ic = 0; ic < buttons[0]->count(); ++ic) {
        dlg.m_combo->addItem(buttons[0]->itemText(ic));
    }
    dlg.m_combo->setCurrentText(current_text);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    current_text = dlg.m_combo->currentText();

    for (int ic = 0; ic < buttons.size(); ++ic) {
        buttons[ic]->blockSignals(true);
        buttons[ic]->setCurrentText(current_text);
        button_items[ic]->setText(current_text);
        buttons[ic]->blockSignals(false);
    }
}

void ResultOutputDlg::targetListRelatedFileVoxFdtd()
{
    std::vector<QComboBox*>        buttons;
    std::vector<QTableWidgetItem*> button_items;
    QItemSelectionModel*           selection_model = ui->tableWidget_target_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_target_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            const QString& ext = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_TYPE)->text();

            if (ext == "vox") {
                buttons.emplace_back(
                    (QComboBox*)ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE));
                button_items.emplace_back(
                    ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE));
            }
        }
    }

    if (buttons.size() == 0) {
        return;
    }

    QString current_text = buttons[0]->currentText();
    /*
    for (int ic = 1; ic < buttons.size(); ++ic) {
        if (current_text != buttons[ic]->currentText()) {
            current_text = "(None)";
            break;
        }
    }*/

    ComboBoxDialog dlg("Related File vox - fdtd", this);
    for (int ic = 0; ic < buttons[0]->count(); ++ic) {
        dlg.m_combo->addItem(buttons[0]->itemText(ic));
    }
    dlg.m_combo->setCurrentText(current_text);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    current_text = dlg.m_combo->currentText();

    for (int ic = 0; ic < buttons.size(); ++ic) {
        buttons[ic]->blockSignals(true);
        buttons[ic]->setCurrentText(current_text);
        button_items[ic]->setText(current_text);
        buttons[ic]->blockSignals(false);
    }
}

void ResultOutputDlg::updateOptSurfaceScale()
{
    if (ui->radioButton_opt_real_scale->isChecked()) {
        int x_decimals = 0;
        int y_decimals = 0;
        int z_decimals = 0;

        float x_step = 1.0f;
        float y_step = 1.0f;
        float z_step = 1.0f;

        auto voxel = m_3DForm->resultCtl()->opt3D();
        if (voxel) {
            VoxelScalar* voxel_scalar = voxel->object<VoxelScalar>();
            if (voxel_scalar) {
                x_decimals = valueDecimals(voxel_scalar->originalDX() * 1e3f);
                y_decimals = valueDecimals(voxel_scalar->originalDY() * 1e3f);
                z_decimals = valueDecimals(voxel_scalar->originalDZ() * 1e3f);
                x_step     = voxel_scalar->originalDX() * 1e3f;
                y_step     = voxel_scalar->originalDY() * 1e3f;
                z_step     = voxel_scalar->originalDZ() * 1e3f;
            }
        }

        ui->sp_min_xy->setDecimals(z_decimals);
        ui->sp_min_yz->setDecimals(x_decimals);
        ui->sp_min_xz->setDecimals(y_decimals);
        ui->sp_max_xy->setDecimals(z_decimals);
        ui->sp_max_yz->setDecimals(x_decimals);
        ui->sp_max_xz->setDecimals(y_decimals);
        ui->sp_step_xy->setDecimals(z_decimals);
        ui->sp_step_yz->setDecimals(x_decimals);
        ui->sp_step_xz->setDecimals(y_decimals);
        ui->sp_step_xy->setSingleStep(x_step);
        ui->sp_step_yz->setSingleStep(y_step);
        ui->sp_step_xz->setSingleStep(z_step);
    }
    else {
        ui->sp_min_xy->setDecimals(0);
        ui->sp_min_yz->setDecimals(0);
        ui->sp_min_xz->setDecimals(0);
        ui->sp_max_xy->setDecimals(0);
        ui->sp_max_yz->setDecimals(0);
        ui->sp_max_xz->setDecimals(0);
        ui->sp_step_xy->setDecimals(0);
        ui->sp_step_yz->setDecimals(0);
        ui->sp_step_xz->setDecimals(0);
        ui->sp_step_xy->setSingleStep(1);
        ui->sp_step_yz->setSingleStep(1);
        ui->sp_step_xz->setSingleStep(1);
    }
}

void ResultOutputDlg::changeOptSurfaceScale()
{
    /// update前
    std::set<float> current_xy_data = strToSurfaceValue(ui->le_xy->text(), ui->sp_min_xy->decimals());
    std::set<float> current_yz_data = strToSurfaceValue(ui->le_yz->text(), ui->sp_min_yz->decimals());
    std::set<float> current_xz_data = strToSurfaceValue(ui->le_xz->text(), ui->sp_min_xz->decimals());

    /// update
    updateOptSurfaceScale();

    /// update後
    float x_step = 1.0f;
    float y_step = 1.0f;
    float z_step = 1.0f;

    auto voxel = m_3DForm->resultCtl()->opt3D();
    if (voxel) {
        VoxelScalar* voxel_scalar = voxel->object<VoxelScalar>();
        if (voxel_scalar) {
            if (ui->radioButton_opt_real_scale->isChecked()) {
                x_step = voxel_scalar->originalDX() * 1e3f;
                y_step = voxel_scalar->originalDY() * 1e3f;
                z_step = voxel_scalar->originalDZ() * 1e3f;
            }
            else {
                x_step = 1.0f / (voxel_scalar->originalDX() * 1e3f);
                y_step = 1.0f / (voxel_scalar->originalDY() * 1e3f);
                z_step = 1.0f / (voxel_scalar->originalDZ() * 1e3f);
            }
        }
    }

    if (x_step != 1.0f) {
        ui->sp_min_yz->setValue(ui->sp_min_yz->value() * x_step);
        ui->sp_max_yz->setValue(ui->sp_max_yz->value() * x_step);
        ui->sp_step_yz->setValue(ui->sp_step_yz->value() * x_step);

        std::set<float> modified_data;
        for (auto& data : current_yz_data) {
            modified_data.insert(data * x_step);
        }
        ui->le_yz->setText(summarizeValues(modified_data, ui->sp_min_yz->decimals()));
    }
    if (y_step != 1.0f) {
        ui->sp_min_xz->setValue(ui->sp_min_xz->value() * y_step);
        ui->sp_max_xz->setValue(ui->sp_max_xz->value() * y_step);
        ui->sp_step_xz->setValue(ui->sp_step_xz->value() * y_step);

        std::set<float> modified_data;
        for (auto& data : current_xz_data) {
            modified_data.insert(data * y_step);
        }
        ui->le_xz->setText(summarizeValues(modified_data, ui->sp_min_xz->decimals()));
    }
    if (z_step != 1.0f) {
        ui->sp_min_xy->setValue(ui->sp_min_xy->value() * z_step);
        ui->sp_max_xy->setValue(ui->sp_max_xy->value() * z_step);
        ui->sp_step_xy->setValue(ui->sp_step_xy->value() * z_step);

        std::set<float> modified_data;
        for (auto& data : current_xy_data) {
            modified_data.insert(data * z_step);
        }
        ui->le_xy->setText(summarizeValues(modified_data, ui->sp_min_xy->decimals()));
    }
}

void ResultOutputDlg::addValuesToLineEdit(QCheckBox* checkBox, QDoubleSpinBox* minSpinBox, QDoubleSpinBox* maxSpinBox,
                                          QDoubleSpinBox* stepSpinBox, QLineEdit* lineEdit)
{
    checkBox->setChecked(true);

    QString s;
    int     decimals = minSpinBox->decimals();

    float min  = minSpinBox->value();
    float max  = maxSpinBox->value();
    float step = stepSpinBox->value();

    if (lineEdit->text().isEmpty()) {
        s = "";
    }
    else {
        s = lineEdit->text() + ",";
    }

    if (min <= max && step > 0) {
        for (float n = min; n <= max; n += step) {
            s = s + QString::number(n, 'f', decimals) + ",";
        }
    }
    else {
        s = s + QString::number(min);
    }

    lineEdit->setText(correctInputOptSurfaceValue(s, decimals));
}

void ResultOutputDlg::oneDirectionXY_XAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_xy_x, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_xy_x);
}

void ResultOutputDlg::oneDirectionXY_YAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_xy_y, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_xy_y);
}

void ResultOutputDlg::oneDirectionYZ_YAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_yz_y, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_yz_y);
}

void ResultOutputDlg::oneDirectionYZ_ZAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_yz_z, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_yz_z);
}

void ResultOutputDlg::oneDirectionXZ_XAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_xz_x, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_xz_x);
}

void ResultOutputDlg::oneDirectionXZ_ZAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_xz_z, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_xz_z);
}

void ResultOutputDlg::oneDirectionDZ_DAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_dz_d, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_dz_d);
}

void ResultOutputDlg::oneDirectionDZ_ZAdd()
{
    addValuesToLineEdit(ui->checkBox_1d_dz_z, ui->sp_min_1d, ui->sp_max_1d, ui->sp_step_1d, ui->le_1d_dz_z);
}

void ResultOutputDlg::oneDirectionXY_XUpdate()
{
    if (ui->checkBox_1d_xy_x->isChecked()) {
        ui->le_1d_xy_x->setEnabled(true);
    }
    else {
        ui->le_1d_xy_x->setEnabled(false);
    }
}

void ResultOutputDlg::oneDirectionXY_YUpdate()
{
    if (ui->checkBox_1d_xy_y->isChecked()) {
        ui->le_1d_xy_y->setEnabled(true);
    }
    else {
        ui->le_1d_xy_y->setEnabled(false);
    }
}

void ResultOutputDlg::oneDirectionYZ_YUpdate()
{
    if (ui->checkBox_1d_yz_y->isChecked()) {
        ui->le_1d_yz_y->setEnabled(true);
    }
    else {
        ui->le_1d_yz_y->setEnabled(false);
    }
}

void ResultOutputDlg::oneDirectionYZ_ZUpdate()
{
    if (ui->checkBox_1d_yz_z->isChecked()) {
        ui->le_1d_yz_z->setEnabled(true);
    }
    else {
        ui->le_1d_yz_z->setEnabled(false);
    }
}

void ResultOutputDlg::oneDirectionXZ_XUpdate()
{
    if (ui->checkBox_1d_xz_x->isChecked()) {
        ui->le_1d_xz_x->setEnabled(true);
    }
    else {
        ui->le_1d_xz_x->setEnabled(false);
    }
}

void ResultOutputDlg::oneDirectionXZ_ZUpdate()
{
    if (ui->checkBox_1d_xz_z->isChecked()) {
        ui->le_1d_xz_z->setEnabled(true);
    }
    else {
        ui->le_1d_xz_z->setEnabled(false);
    }
}

void ResultOutputDlg::oneDirectionDZ_DUpdate()
{
    if (ui->checkBox_1d_dz_d->isChecked()) {
        ui->le_1d_dz_d->setEnabled(true);
    }
    else {
        ui->le_1d_dz_d->setEnabled(false);
    }
}

void ResultOutputDlg::oneDirectionDZ_ZUpdate()
{
    if (ui->checkBox_1d_dz_z->isChecked()) {
        ui->le_1d_dz_z->setEnabled(true);
    }
    else {
        ui->le_1d_dz_z->setEnabled(false);
    }
}

void ResultOutputDlg::update1dDirectionScale()
{
    if (ui->radioButton_1d_real_scale->isChecked()) {
        int   decimals = 0;
        float step     = 1.0f;

        Point3f dxdydz = m_3DForm->resultCtl()->minDxDyDz();
        if (dxdydz[0] != FLT_MAX) {
            float delta = std::min(dxdydz[0], std::min(dxdydz[1], dxdydz[2]));
            decimals    = valueDecimals(delta * 1e3f);
            step        = delta * 1e3f;
        }

        ui->sp_min_1d->setDecimals(decimals);
        ui->sp_max_1d->setDecimals(decimals);
        ui->sp_step_1d->setDecimals(decimals);
        ui->sp_step_1d->setSingleStep(step);
    }
    else {
        ui->sp_min_1d->setDecimals(0);
        ui->sp_max_1d->setDecimals(0);
        ui->sp_step_1d->setDecimals(0);
        ui->sp_step_1d->setSingleStep(1);
    }
}

void ResultOutputDlg::change1dDirectionScale()
{
    /// update前
    std::set<float> current_xy_x_data = strToSurfaceValue(ui->le_1d_xy_x->text(), ui->sp_min_1d->decimals());
    std::set<float> current_xy_y_data = strToSurfaceValue(ui->le_1d_xy_y->text(), ui->sp_min_1d->decimals());
    std::set<float> current_yz_y_data = strToSurfaceValue(ui->le_1d_yz_y->text(), ui->sp_min_1d->decimals());
    std::set<float> current_yz_z_data = strToSurfaceValue(ui->le_1d_yz_z->text(), ui->sp_min_1d->decimals());
    std::set<float> current_xz_x_data = strToSurfaceValue(ui->le_1d_xz_x->text(), ui->sp_min_1d->decimals());
    std::set<float> current_xz_z_data = strToSurfaceValue(ui->le_1d_xz_z->text(), ui->sp_min_1d->decimals());
    std::set<float> current_dz_d_data = strToSurfaceValue(ui->le_1d_dz_d->text(), ui->sp_min_1d->decimals());
    std::set<float> current_dz_z_data = strToSurfaceValue(ui->le_1d_dz_z->text(), ui->sp_min_1d->decimals());

    /// update
    update1dDirectionScale();

    /// update後
    float step = 1.0f;

    Point3f dxdydz = m_3DForm->resultCtl()->minDxDyDz();
    if (dxdydz[0] != FLT_MAX) {
        float delta = std::min(dxdydz[0], std::min(dxdydz[1], dxdydz[2]));
        step        = delta * 1e3f;

        if (ui->radioButton_1d_real_scale->isChecked()) {
            step = delta * 1e3f;
        }
        else {
            step = 1.0f / (delta * 1e3f);
        }
    }

    if (step != 1.0f) {
        ui->sp_min_1d->setValue(ui->sp_min_1d->value() * step);
        ui->sp_max_1d->setValue(ui->sp_max_1d->value() * step);
        ui->sp_step_1d->setValue(ui->sp_step_1d->value() * step);

        // current_xy_x_dataに対する処理
        std::set<float> modified_xy_x_data;
        for (auto& data : current_xy_x_data) {
            modified_xy_x_data.insert(data * step);
        }
        ui->le_1d_xy_x->setText(summarizeValues(modified_xy_x_data, ui->sp_min_1d->decimals()));

        // current_xy_y_dataに対する処理
        std::set<float> modified_xy_y_data;
        for (auto& data : current_xy_y_data) {
            modified_xy_y_data.insert(data * step);
        }
        ui->le_1d_xy_y->setText(summarizeValues(modified_xy_y_data, ui->sp_min_1d->decimals()));

        // current_yz_y_dataに対する処理
        std::set<float> modified_yz_y_data;
        for (auto& data : current_yz_y_data) {
            modified_yz_y_data.insert(data * step);
        }
        ui->le_1d_yz_y->setText(summarizeValues(modified_yz_y_data, ui->sp_min_1d->decimals()));

        // current_yz_z_dataに対する処理
        std::set<float> modified_yz_z_data;
        for (auto& data : current_yz_z_data) {
            modified_yz_z_data.insert(data * step);
        }
        ui->le_1d_yz_z->setText(summarizeValues(modified_yz_z_data, ui->sp_min_1d->decimals()));

        // current_xz_x_dataに対する処理
        std::set<float> modified_xz_x_data;
        for (auto& data : current_xz_x_data) {
            modified_xz_x_data.insert(data * step);
        }
        ui->le_1d_xz_x->setText(summarizeValues(modified_xz_x_data, ui->sp_min_1d->decimals()));

        // current_xz_z_dataに対する処理
        std::set<float> modified_xz_z_data;
        for (auto& data : current_xz_z_data) {
            modified_xz_z_data.insert(data * step);
        }
        ui->le_1d_xz_z->setText(summarizeValues(modified_xz_z_data, ui->sp_min_1d->decimals()));

        // current_dz_d_dataに対する処理
        std::set<float> modified_dz_d_data;
        for (auto& data : current_dz_d_data) {
            modified_dz_d_data.insert(data * step);
        }
        ui->le_1d_dz_d->setText(summarizeValues(modified_dz_d_data, ui->sp_min_1d->decimals()));

        // current_dz_z_dataに対する処理
        std::set<float> modified_dz_z_data;
        for (auto& data : current_dz_z_data) {
            modified_dz_z_data.insert(data * step);
        }
        ui->le_1d_dz_z->setText(summarizeValues(modified_dz_z_data, ui->sp_min_1d->decimals()));
    }
}

std::vector<ResultOutputDlg::PlaneInfo> ResultOutputDlg::optTargetSurface()
{
    std::vector<PlaneInfo> output;

    const auto& xz_values =
        ui->cb_xz->isChecked() ? strToSurfaceValue(ui->le_xz->text(), ui->sp_min_xz->decimals()) : std::set<float>();
    const auto& yz_values =
        ui->cb_yz->isChecked() ? strToSurfaceValue(ui->le_yz->text(), ui->sp_min_yz->decimals()) : std::set<float>();
    const auto& xy_values =
        ui->cb_xy->isChecked() ? strToSurfaceValue(ui->le_xy->text(), ui->sp_min_xy->decimals()) : std::set<float>();

    const std::set<float>* all_data[] = {&xz_values, &yz_values, &xy_values};

    bool real_scale = ui->radioButton_opt_real_scale->isChecked();

    for (int ic = 0; ic < sizeof(all_data) / sizeof(all_data[0]); ++ic) {
        const auto& values = *all_data[ic];
        for (auto value : values) {
            if (value < 0) continue;
            if (real_scale) {
                output.emplace_back(ic, -1, value * 1e-3f);
            }
            else {
                output.emplace_back(ic, value, 0);
            }
        }
    }

    return output;
}

std::vector<std::vector<ResultOutputDlg::OneDirecitonInfo>> ResultOutputDlg::target1DDirection()
{
    std::vector<std::vector<ResultOutputDlg::OneDirecitonInfo>> output_all(4);

    const auto& xz_x_values = ui->checkBox_1d_xz_x->isChecked()
                                ? strToSurfaceValue(ui->le_1d_xz_x->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();
    const auto& xz_z_values = ui->checkBox_1d_xz_z->isChecked()
                                ? strToSurfaceValue(ui->le_1d_xz_z->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();

    const auto& yz_y_values = ui->checkBox_1d_yz_y->isChecked()
                                ? strToSurfaceValue(ui->le_1d_yz_y->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();
    const auto& yz_z_values = ui->checkBox_1d_yz_z->isChecked()
                                ? strToSurfaceValue(ui->le_1d_yz_z->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();

    const auto& xy_x_values = ui->checkBox_1d_xy_x->isChecked()
                                ? strToSurfaceValue(ui->le_1d_xy_x->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();
    const auto& xy_y_values = ui->checkBox_1d_xy_y->isChecked()
                                ? strToSurfaceValue(ui->le_1d_xy_y->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();

    const auto& dz_d_values = ui->checkBox_1d_dz_d->isChecked()
                                ? strToSurfaceValue(ui->le_1d_dz_d->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();
    const auto& dz_z_values = ui->checkBox_1d_dz_z->isChecked()
                                ? strToSurfaceValue(ui->le_1d_dz_z->text(), ui->sp_min_1d->decimals())
                                : std::set<float>();

    const std::set<float>* all_data[] = {&xz_x_values, &xz_z_values, &yz_y_values, &yz_z_values,
                                         &xy_x_values, &xy_y_values, &dz_d_values, &dz_z_values};

    bool is_height[] = {
        true, false,    /// xz_x, xz_z
        true, false,    /// yz_y, yz_z
        true, false,    /// xy_x, xy_y
        true, false     /// dz_d, dz_z
    };

    bool real_scale = ui->radioButton_1d_real_scale->isChecked();

    for (int ic = 0; ic < sizeof(all_data) / sizeof(all_data[0]); ++ic) {
        auto& output = output_all[ic / 2];

        const auto& values = *all_data[ic];
        bool        is_h   = is_height[ic];

        for (auto value : values) {
            if (value < 0) continue;
            if (real_scale) {
                output.emplace_back(ic / 2, is_h, -1, value * 1e-3f);
            }
            else {
                output.emplace_back(ic / 2, is_h, value, 0);
            }
        }
    }

    return output_all;
}

void ResultOutputDlg::setDefault()
{
    SettingGuiCtrl* setting_gui_ctrl = m_3DForm->settingGuiCtrl();
    if (!setting_gui_ctrl) {
        return;
    }

    if (QMessageBox::question(this, windowTitle(), "現在のImage Settingをデフォルトに登録します。")
        != QMessageBox::Yes) {
        return;
    }

    int picture_ext =
        ui->radioButton_pictureformat_png->isChecked() ? 0 : (ui->radioButton_pictureformat_jpg->isChecked() ? 1 : 2);
    setting_gui_ctrl->saveResultOutputImageFormat(picture_ext);

    int size_type =
        ui->radioButton_picture_size_data->isChecked() ? 0 : (ui->radioButton_picture_size_view->isChecked() ? 1 : 2);
    setting_gui_ctrl->saveResultOutputImageSizeType(size_type);

    setting_gui_ctrl->saveResultOutputImageSizeRatio(ui->doubleSpinBox_picture_size_data->value());
    setting_gui_ctrl->saveResultOutputImageSizeW(ui->spinBox_picture_size_specify_w->value());
    setting_gui_ctrl->saveResultOutputImageSizeH(ui->spinBox_picture_size_specify_h->value());

    int colorval_type = ui->radioButton_colormap_value_current->isChecked()
                          ? 0
                          : (ui->radioButton_colormap_value_specify->isChecked() ? 1 : 2);
    setting_gui_ctrl->saveResultOutputImageColorvalType(colorval_type);

    setting_gui_ctrl->saveResultOutputImageColorvalMin(ui->lineEdit_colormap_specify_min->text());
    setting_gui_ctrl->saveResultOutputImageColorvalMax(ui->lineEdit_colormap_specify_max->text());

    setting_gui_ctrl->saveResultOutputImageColorbar(ui->checkBox_output_colorbar->isChecked());
    setting_gui_ctrl->saveResultOutputImageSpecifyRange(ui->checkBox_specify_range_setting->isChecked());
    setting_gui_ctrl->saveResultOutputImageDispVox(ui->checkBox_outline_vox_show_hide->isChecked());

    setting_gui_ctrl->saveResultOutputImageBackgroundChange(ui->checkBox_change_background->isChecked());
    setting_gui_ctrl->saveResultOutputImageBackgroundGrad(ui->checkBox_background_grad->isChecked());
    setting_gui_ctrl->saveResultOutputImageBackgroundColor(m_background_color);
}

void ResultOutputDlg::applyDefault(bool show_msg)
{
    SettingGuiCtrl* setting_gui_ctrl = m_3DForm->settingGuiCtrl();
    if (!setting_gui_ctrl) {
        return;
    }

    if (show_msg) {
        if (QMessageBox::question(this, windowTitle(), "Image Settingをデフォルトに戻します。") != QMessageBox::Yes) {
            return;
        }
    }

    int picture_ext = setting_gui_ctrl->readResultOutputImageFormat();
    switch (picture_ext) {
        case 0:
            ui->radioButton_pictureformat_png->setChecked(true);
            break;
        case 1:
            ui->radioButton_pictureformat_jpg->setChecked(true);
            break;
        case 2:
            ui->radioButton_pictureformat_bmp->setChecked(true);
            break;
    }

    int size_type = setting_gui_ctrl->readResultOutputImageSizeType();
    switch (size_type) {
        case 0:
            ui->radioButton_picture_size_data->setChecked(true);
            break;
        case 1:
            ui->radioButton_picture_size_view->setChecked(true);
            break;
        case 2:
            ui->radioButton_picture_size_specify->setChecked(true);
            break;
    }

    ui->doubleSpinBox_picture_size_data->setValue(setting_gui_ctrl->readResultOutputImageSizeRatio());
    ui->spinBox_picture_size_specify_w->setValue(setting_gui_ctrl->readResultOutputImageSizeW());
    ui->spinBox_picture_size_specify_h->setValue(setting_gui_ctrl->readResultOutputImageSizeH());

    int colorval_type = setting_gui_ctrl->readResultOutputImageColorvalType();
    switch (colorval_type) {
        case 0:
            ui->radioButton_colormap_value_current->setChecked(true);
            break;
        case 1:
            ui->radioButton_colormap_value_specify->setChecked(true);
            break;
        case 2:
            ui->radioButton_colormap_value_minmax->setChecked(true);
            break;
    }

    ui->lineEdit_colormap_specify_min->setText(setting_gui_ctrl->readResultOutputImageColorvalMin());
    ui->lineEdit_colormap_specify_max->setText(setting_gui_ctrl->readResultOutputImageColorvalMax());

    ui->checkBox_output_colorbar->setChecked(setting_gui_ctrl->readResultOutputImageColorbar());
    ui->checkBox_specify_range_setting->setChecked(setting_gui_ctrl->readResultOutputImageSpecifyRange());
    ui->checkBox_outline_vox_show_hide->setChecked(setting_gui_ctrl->readResultOutputImageDispVox());

    ui->checkBox_change_background->setChecked(setting_gui_ctrl->readResultOutputImageBackgroundChange());
    ui->checkBox_background_grad->setChecked(setting_gui_ctrl->readResultOutputImageBackgroundGrad());
    m_background_color = setting_gui_ctrl->readResultOutputImageBackgroundColor();
    ui->pushButton_background_color->setStyleSheet(Vox3DForm::colorStyle(m_background_color));
}

void ResultOutputDlg::initDefault()
{
    SettingGuiCtrl* setting_gui_ctrl = m_3DForm->settingGuiCtrl();
    if (!setting_gui_ctrl) {
        return;
    }

    if (QMessageBox::question(this, windowTitle(), "Image Settingのデフォルト設定を初期化します。")
        != QMessageBox::Yes) {
        return;
    }

    setting_gui_ctrl->initResultOutputImage();
}

void ResultOutputDlg::setProgMessage(int value)
{
    if (m_pd) {
        m_pd->setValue(value);
    }
}

void ResultOutputDlg::setProgText(const QString& text)
{
    if (m_pd) {
        m_pd->setLabelText(text);
    }
}

void ResultOutputDlg::setProgEnd()
{
    if (m_pd) {
        delete m_pd;
        m_pd = nullptr;
    }
}

void ResultOutputDlg::optSurfaceXYAdd()
{
    addValuesToLineEdit(ui->cb_xy, ui->sp_min_xy, ui->sp_max_xy, ui->sp_step_xy, ui->le_xy);
}

void ResultOutputDlg::optSurfaceYZAdd()
{
    addValuesToLineEdit(ui->cb_yz, ui->sp_min_yz, ui->sp_max_yz, ui->sp_step_yz, ui->le_yz);
}

void ResultOutputDlg::optSurfaceZXAdd()
{
    addValuesToLineEdit(ui->cb_xz, ui->sp_min_xz, ui->sp_max_xz, ui->sp_step_xz, ui->le_xz);
}

void ResultOutputDlg::updateDlg()
{
    ui->lineEdit_colormap_current_min->setText(m_3DForm->resultCtl()->colormapMinLabel());
    ui->lineEdit_colormap_current_max->setText(m_3DForm->resultCtl()->colormapMaxLabel());

    int width  = m_gl_widget->sceneView()->viewPortWidth();
    int height = m_gl_widget->sceneView()->viewPortHeight();

    ui->lineEdit_picture_size_view->setText(QString("%1 * %2").arg(width).arg(height));

    updateOptSurfaceScale();

    update1dDirectionScale();
}

void ResultOutputDlg::outputPictureText()
{
    /// とりあえず非表示
    hideFileExplorerDlg();

    bool output_image   = ui->checkBox_2d_target_image->isChecked();
    bool output_text    = ui->checkBox_2d_target_text->isChecked();
    bool output_1d_text = ui->checkBox_1d_target_text->isChecked();
    if (!output_image && !output_text && !output_1d_text) {
        QMessageBox::warning(this, windowTitle(), "Image, 2D Text, or 1D Text is not checked.");
        return;
    }

    QString output_folder;
    if (ui->radioButton_output2d_output_folder->isChecked()) {
        output_folder = ui->lineEdit_output2d_output_folder->text();
        if (output_folder.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), "Output folder is not set.");
            return;
        }
        if (!QFileInfo(output_folder).exists()) {
            QMessageBox::warning(this, windowTitle(), "Output folder does not exist.");
            return;
        }
    }

    ResultOutputThread::Input input;

    std::vector<ResultOutputThread::InputGroup>& input_groups = input.m_input_groups;
    if (ui->radioButton_target_folder->isChecked()) {
        std::map<QString, ResultOutputThread::InputGroup> map_group;

        for (int row = 0; row < ui->tableWidget_target_list->rowCount(); ++row) {
            const QString& ext  = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_TYPE)->text();
            const QString& path = ui->tableWidget_target_list->item(row, RESULT_OUTPUT_TARGET_LIST_FILE_FULLPATH)
                                      ->data(Qt::UserRole)
                                      .toString();

            if (ext == "op2" || ext == "opt") {
                auto cell_widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_TARGET);
                if (cell_widget) {
                    QCheckBox* check_box = (QCheckBox*)cell_widget->findChild<QCheckBox*>();
                    if (!check_box->isChecked()) {
                        continue;
                    }
                }
            }

            if (ext == "vox") {
                QString vox_file = path;

                auto& group      = map_group[vox_file.toLower()];
                group.m_vox_file = path;

                QString fdtd_file;

                auto cell_widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE);
                if (cell_widget) {
                    fdtd_file = ((QComboBox*)cell_widget)->currentText();
                    if (fdtd_file == "(None)") {
                        fdtd_file = "";
                    }
                }

                group.m_fdtd_file = fdtd_file;
            }
            else if (ext == "opt") {
                QString vox_file;

                auto cell_widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE);
                if (cell_widget) {
                    vox_file = ((QComboBox*)cell_widget)->currentText();
                    if (vox_file == "(None)") {
                        vox_file = "";
                    }
                }

                map_group[vox_file.toLower()].m_opt_list << path;
            }
            else if (ext == "op2") {
                QString vox_file;

                auto cell_widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_RELATED_FILE);
                if (cell_widget) {
                    vox_file = ((QComboBox*)cell_widget)->currentText();
                    if (vox_file == "(None)") {
                        vox_file = "";
                    }
                }

                int      array_x = -1;
                int      array_y = -1;
                QWidget* widget  = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_ARRAY_X);
                if (widget) {
                    QSpinBox* spin_box = (QSpinBox*)(widget);
                    array_x            = spin_box->value();
                }
                widget = ui->tableWidget_target_list->cellWidget(row, RESULT_OUTPUT_TARGET_LIST_ARRAY_Y);
                if (widget) {
                    QSpinBox* spin_box = (QSpinBox*)(widget);
                    array_y            = spin_box->value();
                }

                auto& group = map_group[vox_file.toLower()];

                group.m_op2_list << path;
                group.m_op2_array_list << QPair<int, int>(array_x, array_y);
            }
        }

        for (auto& [key, group] : map_group) {
            if (group.m_op2_list.size() > 0 || group.m_opt_list.size() > 0) {
                input_groups.emplace_back(group);
            }
        }

        /// データなし
        if (input_groups.size() == 0) {
            QMessageBox::warning(this, windowTitle(), "There is no target.");
            return;
        }
    }

    bool  change_colormap_specify = false;
    bool  change_colormap_minmax  = false;
    bool  colormap_valid          = false;
    float colormap_min            = FLT_MAX;
    float colormap_max            = -FLT_MAX;
    if (output_image) {
        if (ui->radioButton_colormap_value_current->isChecked()) {
            bool  ok   = false;
            float temp = m_3DForm->resultCtl()->colormapMinLabel().toFloat(&ok);
            if (ok) {
                colormap_min = temp;
            }
            ok   = false;
            temp = m_3DForm->resultCtl()->colormapMaxLabel().toFloat(&ok);
            if (ok) {
                colormap_max = temp;
            }
            if (colormap_min < colormap_max) {
                colormap_valid = true;
            }
        }
        else if (ui->radioButton_colormap_value_specify->isChecked()) {
            bool  ok   = false;
            float temp = ui->lineEdit_colormap_specify_min->text().toFloat(&ok);
            if (ok) {
                colormap_min = temp;
            }
            ok   = false;
            temp = ui->lineEdit_colormap_specify_max->text().toFloat(&ok);
            if (ok) {
                colormap_max = temp;
            }
            if (colormap_min < colormap_max) {
                colormap_valid          = true;
                change_colormap_specify = true;
            }
        }
        else if (ui->radioButton_colormap_value_minmax->isChecked()) {
            colormap_valid         = true;
            change_colormap_minmax = true;
        }
        if (!colormap_valid) {
            QMessageBox::warning(this, windowTitle(), "Picture colormap value is invalid.");
            return;
        }
    }

    bool specify_range =
        ui->checkBox_specify_range_setting->isChecked() && m_3DForm->resultCtl()->isEnableSpecifiedRange();
    bool output_colorbar = ui->checkBox_output_colorbar->isChecked();

    QString picture_ext = ui->radioButton_pictureformat_png->isChecked()
                            ? ".png"
                            : (ui->radioButton_pictureformat_jpg->isChecked() ? ".jpg" : ".bmp");

    float data_size_ratio = 0;
    int   pic_width       = 0;
    int   pic_height      = 0;
    if (output_image) {
        if (ui->radioButton_picture_size_data->isChecked()) {
            data_size_ratio = ui->doubleSpinBox_picture_size_data->value();
        }
        else if (ui->radioButton_picture_size_specify->isChecked()) {
            ui->doubleSpinBox_picture_size_data->setEnabled(false);
            pic_width  = ui->spinBox_picture_size_specify_w->value();
            pic_height = ui->spinBox_picture_size_specify_h->value();
        }
        else {
            pic_width  = m_gl_widget->sceneView()->viewPortWidth();
            pic_height = m_gl_widget->sceneView()->viewPortHeight();
        }

        if (data_size_ratio > 0) {
        }
        else if (pic_width <= 0 || pic_height <= 0) {
            QMessageBox::warning(this, windowTitle(), "Picture size is invalid.");
            return;
        }
    }

    m_pd = new QProgressDialog(this);
    m_pd->setMinimumDuration(500);
    Qt::WindowFlags flags = m_pd->windowFlags();
    flags &= ~Qt::WindowCloseButtonHint;
    m_pd->setWindowFlags(flags);
    m_pd->setLabelText("");
    m_pd->setModal(true);
    m_pd->open();
    m_pd->setFixedSize(m_pd->sizeHint());
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    bool thread_end    = false;
    bool thread_cancel = false;
    connect(m_pd, &QProgressDialog::canceled, [this, &thread_cancel]() {
        if (m_pd) {
            thread_cancel = true;
            m_pd->setLabelText("Canceling...");
            /// 以降キャンセルできないのでキャンセルボタンをグレーアウト
            QList<QPushButton*> buttons = m_pd->findChildren<QPushButton*>();
            for (auto btn : buttons) {
                if (!btn->text().isEmpty()) {
                    btn->setEnabled(false);    // グレーアウト
                }
            }
            /// キャンセルで消えてしまうので再表示
            m_pd->show();
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    });

    bool outline_current_show = ui->checkBox_outline_vox_show_hide->isChecked();

    /// Voxの表示
    bool current_shading, current_wireframe;
    m_3DForm->currentVoxDrawMode(current_shading, current_wireframe);

    input.m_target_only_disp        = ui->radioButton_target_current_disp->isChecked();
    input.m_output_image            = output_image;
    input.m_output_text             = output_text;
    input.m_output_1d_text          = output_1d_text;
    input.m_change_colormap_minmax  = change_colormap_minmax;
    input.m_change_colormap_specify = change_colormap_specify;
    input.m_colormap_valid          = colormap_valid;
    input.m_colormap_max            = colormap_max;
    input.m_colormap_min            = colormap_min;
    input.m_specify_range           = specify_range;
    input.m_output_colorbar         = output_colorbar;
    input.m_ext                     = picture_ext;
    input.m_data_size_ratio         = data_size_ratio;
    input.m_pic_width               = pic_width;
    input.m_pic_height              = pic_height;
    input.m_outline_current_show    = outline_current_show;
    input.m_vox_current_shading     = current_shading;
    input.m_vox_current_wireframe   = current_wireframe;
    input.m_background_color_change = ui->checkBox_change_background->isChecked();
    input.m_background_color        = m_background_color;
    input.m_background_grad         = ui->checkBox_background_grad->isChecked();
    input.m_target_surface          = optTargetSurface();
    if (output_1d_text) {
        input.m_target_one_direction = target1DDirection();
    }
    input.m_output_folder = output_folder;

    /// GUI関連のバックアップ
    std::vector<char>               back_compress_data      = m_3DForm->resultCtl()->voxelFileCompressData();
    int                             back_compress_data_size = m_3DForm->resultCtl()->compressDataOriginalSize();
    std::map<int, WeakRefPtr<Node>> material_id_nodes       = m_gl_widget->materialIdNodes();

    ResultOutputThread output_thread(this, m_3DForm, m_gl_widget, input);
    connect(&output_thread, &ResultOutputThread::setProgMessage, this, &ResultOutputDlg::setProgMessage,
            Qt::QueuedConnection);
    connect(&output_thread, &ResultOutputThread::setProgText, this, &ResultOutputDlg::setProgText,
            Qt::QueuedConnection);
    connect(&output_thread, &ResultOutputThread::setProgEnd, this, &ResultOutputDlg::setProgEnd, Qt::QueuedConnection);

    connect(&output_thread, &ResultOutputThread::finished, this, [&thread_end] { thread_end = true; });
    output_thread.start();

    while (1) {
        qApp->processEvents(Vox3DForm::isModalVisible(m_pd) ? QEventLoop::AllEvents
                                                            : QEventLoop::ExcludeUserInputEvents);
        if (thread_end) {
            break;
        }
        if (thread_cancel) {
            output_thread.setCancel();
        }
    }

    /// deleteで残る場合がある
    if (m_pd) {
        m_pd->hide();
        delete m_pd;
    }
    m_pd = nullptr;

    m_3DForm->resultCtl()->setVoxelFileCompressData(back_compress_data);
    m_3DForm->resultCtl()->setCompressDataOriginalSize(back_compress_data_size);
    m_gl_widget->setMaterialIdToNodeDirect(material_id_nodes);

    const auto& folder_files = output_thread.outputFolderFiles();
    if (folder_files.size() > 0) {
        if (!m_file_explorer_dlg) {
            QString labelText = "以下に作成しました。ダブルクリックでフォルダ/ファイルを開きます。\n"
                                "　※ファイル右クリックメニューからエクスプローラーで開けます。";
            m_file_explorer_dlg = new FileExplorerDialog(labelText, this->windowTitle(), m_3DForm);
        }
        m_file_explorer_dlg->setFolderFileMap(folder_files);
        m_file_explorer_dlg->show();

        if (output_thread.isFailed()) {
            QMessageBox::warning(m_file_explorer_dlg, windowTitle(),
                                 "出力に失敗したファイルがあります。リストで確認してください。");
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(), "There is no target.");
    }
}

void ResultOutputThread::collectTarget(Node* node)
{
    TargetNode target;
    target.m_node        = node;
    target.m_show        = node->isShow();
    target.m_shading     = false;
    target.m_wireframe   = false;
    target.m_transparent = 1.0f;
    target.m_proj_opt    = nullptr;

    if (m_input.m_target_only_disp) {
        if (!node->isVisible()) {
            /// Voxelは除外
            if (!node->object<Voxel>()) {
                return;
            }
        }
    }

    auto object = node->object();
    if (object) {
        switch (object->type()) {
            case ObjectType::Voxel:
                target.m_shading     = ((Voxel*)object)->isDrawShading();
                target.m_wireframe   = ((Voxel*)object)->isDrawWireframe();
                target.m_transparent = ((Voxel*)object)->transparent();
                target.m_proj_opt    = ((Voxel*)object)->projectionOpt();
                m_voxel_list.emplace_back(target);
                return;
            case ObjectType::VoxelScalar:
                if (((VoxelScalar*)object)->is2DTexture()) {
                    m_opt2d_list.emplace_back(target);
                }
                else {
                    m_opt3d_list.emplace_back(target);
                }
                return;
            default:
                m_other_list.emplace_back(target);
                break;
        }
    }

    for (auto& child : node->children()) {
        collectTarget(child.ptr());
    }
}

void ResultOutputThread::run()
{
    outputPictureText();
}

void ResultOutputThread::outputPictureText()
{
    m_gl_widget->suppressRender(true);

    auto save_range_voxels = m_3DForm->resultCtl()->voxelRangeVoxels();
    m_3DForm->resultCtl()->clearVoxelRangeVoxels();

    bool  is_suppress_opt = false;
    Node* opt_3d          = m_3DForm->resultCtl()->opt3D();
    if (opt_3d) {
        is_suppress_opt = opt_3d->isSuppress();
        if (is_suppress_opt) {
            opt_3d->unsuppress();
        }
    }

    bool output_picture = m_input.m_output_image;
    bool output_text    = m_input.m_output_text;
    bool output_1d_text = m_input.m_output_1d_text;

    if (!m_input.m_input_groups.empty()) {
        int output_group_count    = 0;
        int output_group_all_size = 0;
        for (int ic = 0; ic < m_input.m_input_groups.size(); ++ic) {
            auto& group = m_input.m_input_groups[ic];
            output_group_all_size += group.m_opt_list.size();
            if (group.m_op2_list.size() > 0) {
                ++output_group_all_size;
            }
        }

        for (int ic = 0; ic < m_input.m_input_groups.size(); ++ic) {
            if (m_cancel) {
                break;
            }
            m_gl_widget->sceneView()->sceneGraph()->resetTemporaryRoot();
            m_gl_widget->sceneView()->sceneGraph()->setTemporaryRoot();    /// 現在のデータを隠して空にする

            /// ファイルロード
            auto& group = m_input.m_input_groups[ic];

            BoundingBox3f vox_box;    /// なぜかデフォルトがvalidになった(FLT_MAX/-FLT_MAXが判定おかしい？）ので対策
            vox_box.set(0, 0, 0, 1, 1, 1);

            if (output_picture) {
                if (!group.m_vox_file.isEmpty()) {
                    ReadVoxel read_voxel(this, nullptr);
                    if (!group.m_fdtd_file.isEmpty()) {
                        read_voxel.readFdtd(group.m_fdtd_file);
                    }
                    read_voxel.setNoChangeGUI();
                    read_voxel.read(group.m_vox_file, m_gl_widget, m_3DForm);

                    m_voxel_list.clear();
                    m_opt2d_list.clear();
                    m_opt3d_list.clear();
                    m_other_list.clear();

                    auto root_node = m_gl_widget->sceneView()->sceneGraph()->rootNode();
                    collectTarget(root_node);

                    if (m_voxel_list.size() > 0) {
                        vox_box = m_gl_widget->sceneView()->sceneGraph()->rootNode()->boundingBox();
                    }

                    if (m_input.m_outline_current_show) {
                        for (auto& vox : m_voxel_list) {
                            Voxel* voxel = vox.m_node->object<Voxel>();
                            voxel->setDrawShading(m_input.m_vox_current_shading);
                            voxel->setDrawWireframe(m_input.m_vox_current_wireframe);
                        }
                    }

                    m_voxel_list.clear();
                    m_opt2d_list.clear();
                    m_opt3d_list.clear();
                    m_other_list.clear();
                }
            }

            /// Optは個別ロード
            for (auto& opt : group.m_opt_list) {
                ReadOpt read_opt(this, nullptr);
                read_opt.setNoChangeGUI();
                read_opt.read(opt.toStdWString(), m_gl_widget, m_3DForm);

                m_voxel_list.clear();
                m_opt2d_list.clear();
                m_opt3d_list.clear();
                m_other_list.clear();

                auto root_node = m_gl_widget->sceneView()->sceneGraph()->rootNode();
                collectTarget(root_node);

                ++output_group_count;

                if (m_opt3d_list.size() == 0) {
                    m_voxel_list.clear();
                    m_opt2d_list.clear();
                    m_opt3d_list.clear();
                    m_other_list.clear();
                    continue;
                }

                if (m_cancel) {
                    break;
                }

                if (output_picture) {
                    auto threshold_values = m_3DForm->resultCtl()->threshold();
                    auto color_values     = m_3DForm->resultCtl()->colors();

                    for (auto& opt_load : m_opt3d_list) {
                        QMetaObject::invokeMethod(
                            this,
                            [this, &threshold_values, &color_values, &opt_load]() {
                                m_gl_widget->set3DTextureColor(threshold_values, color_values);
                                m_gl_widget->create3DTexture(opt_load.m_node->object<VoxelScalar>());
                                // voxel_scalar->setVboUse(false);
                                m_gl_widget->createRenderData(opt_load.m_node->renderable());
                            },
                            Qt::BlockingQueuedConnection);
                        if (m_cancel) {
                            break;
                        }
                    }
                }

                if (m_cancel) {
                    break;
                }

                if (m_opt2d_list.size() == 0 && (m_opt3d_list.size() * m_input.m_target_surface.size() == 0)) {
                    for (auto& opt_load : m_opt3d_list) {
                        root_node->removeChild(opt_load.m_node.ptr());
                    }
                    m_voxel_list.clear();
                    m_opt2d_list.clear();
                    m_opt3d_list.clear();
                    m_other_list.clear();
                    continue;
                }

                if (output_picture) {
                    if (m_input.m_specify_range) {
                        m_3DForm->resultCtl()->clearVoxelRangeVoxels();

                        std::vector<std::tuple<float, float, int>> range_list;
                        std::vector<std::pair<float, float>>       range_disp_list;
                        m_3DForm->resultCtl()->createVoxelRangeList(range_list, range_disp_list);

                        if (m_cancel) {
                            break;
                        }

                        std::vector<Node*> opt_nodes;
                        for (auto& opt_load : m_opt3d_list) {
                            opt_nodes.emplace_back(opt_load.m_node.ptr());
                        }
                        QMetaObject::invokeMethod(
                            this,
                            [this, &opt_nodes, &range_list, &range_disp_list]() {
                                bool back_check = m_3DForm->resultCtl()->setShowOnlyVoxMaterialOnlyGUI(
                                    false);    /// フォルダ指定の場合OFFにする（現状全表示なので意味ない）
                                bool ret = m_3DForm->resultCtl()->createVoxelRangeShape(
                                    opt_nodes, range_list, range_disp_list, m_dlg->progressDlg(), true);
                                m_3DForm->resultCtl()->setShowOnlyVoxMaterialOnlyGUI(back_check);
                                if (!ret) {
                                    m_cancel = true;
                                }
                            },
                            Qt::BlockingQueuedConnection);

                        if (m_cancel) {
                            break;
                        }
                    }

                    outputPictureOneGroup(
                        QString("Output Image %1 / %2").arg(output_group_count).arg(output_group_all_size));
                }

                if (output_text) {
                    outputTextOneGroup(
                        QString("Output Text 2D %1 / %2").arg(output_group_count).arg(output_group_all_size));
                }

                if (output_1d_text) {
                    outputText1dOneGroup(
                        QString("Output Text 1D %1 / %2").arg(output_group_count).arg(output_group_all_size));
                }

                for (auto& opt_load : m_opt3d_list) {
                    root_node->removeChild(opt_load.m_node.ptr());
                }
                m_voxel_list.clear();
                m_opt2d_list.clear();
                m_opt3d_list.clear();
                m_other_list.clear();
            }

            if (m_cancel) {
                break;
            }

            /// op2まとめてロード
            if (group.m_op2_list.size() > 0) {
                ReadOp2 op2_read(this, nullptr);
                op2_read.setNoChangeGUI();
                op2_read.setAutoDSection(vox_box, &group.m_op2_array_list);
                op2_read.read(group.m_op2_list, m_gl_widget, m_3DForm);

                ++output_group_count;

                m_voxel_list.clear();
                m_opt2d_list.clear();
                m_opt3d_list.clear();
                m_other_list.clear();

                auto root_node = m_gl_widget->sceneView()->sceneGraph()->rootNode();
                collectTarget(root_node);

                if (output_picture) {
                    auto threshold_values = m_3DForm->resultCtl()->threshold();
                    auto color_values     = m_3DForm->resultCtl()->colors();

                    QMetaObject::invokeMethod(
                        this,
                        [this, &threshold_values, &color_values]() {
                            m_gl_widget->set3DTextureColor(threshold_values, color_values);
                        },
                        Qt::BlockingQueuedConnection);
                    for (auto& opt_load : m_opt2d_list) {
                        QMetaObject::invokeMethod(
                            this,
                            [this, &opt_load]() {
                                m_gl_widget->create2DTexture(opt_load.m_node->object<VoxelScalar>());
                                m_gl_widget->createRenderData(opt_load.m_node->renderable());
                            },
                            Qt::BlockingQueuedConnection);
                    }
                }

                if (m_opt2d_list.size() == 0 && (m_opt3d_list.size() * m_input.m_target_surface.size() == 0)) {
                    for (auto& opt_load : m_opt2d_list) {
                        root_node->removeChild(opt_load.m_node.ptr());
                    }
                    m_voxel_list.clear();
                    m_opt2d_list.clear();
                    m_opt3d_list.clear();
                    m_other_list.clear();
                    continue;
                }

                if (output_picture) {
                    if (m_input.m_specify_range) {
                        m_3DForm->resultCtl()->clearVoxelRangeVoxels();

                        std::vector<std::tuple<float, float, int>> range_list;
                        std::vector<std::pair<float, float>>       range_disp_list;
                        m_3DForm->resultCtl()->createVoxelRangeList(range_list, range_disp_list);

                        if (m_cancel) {
                            break;
                        }

                        std::vector<Node*> opt_nodes;
                        for (auto& opt_load : m_opt2d_list) {
                            opt_nodes.emplace_back(opt_load.m_node.ptr());
                        }
                        QMetaObject::invokeMethod(
                            this,
                            [this, &opt_nodes, &range_list, &range_disp_list]() {
                                bool back_check = m_3DForm->resultCtl()->setShowOnlyVoxMaterialOnlyGUI(
                                    false);    /// フォルダ指定の場合OFFにする（現状全表示なので意味ない）
                                bool ret = m_3DForm->resultCtl()->createVoxelRangeShape(
                                    opt_nodes, range_list, range_disp_list, m_dlg->progressDlg(), true);
                                m_3DForm->resultCtl()->setShowOnlyVoxMaterialOnlyGUI(back_check);
                                if (!ret) {
                                    m_cancel = true;
                                }
                            },
                            Qt::BlockingQueuedConnection);

                        if (m_cancel) {
                            break;
                        }
                    }

                    outputPictureOneGroup(
                        QString("Output Image %1 / %2").arg(output_group_count).arg(output_group_all_size));
                }

                if (output_text) {
                    outputTextOneGroup(
                        QString("Output Text 2D %1 / %2").arg(output_group_count).arg(output_group_all_size));
                }

                if (output_1d_text) {
                    outputText1dOneGroup(
                        QString("Output Text 1D %1 / %2").arg(output_group_count).arg(output_group_all_size));
                }

                for (auto& opt_load : m_opt2d_list) {
                    root_node->removeChild(opt_load.m_node.ptr());
                }
                m_voxel_list.clear();
                m_opt2d_list.clear();
                m_opt3d_list.clear();
                m_other_list.clear();
            }

            m_gl_widget->sceneView()->sceneGraph()->resetTemporaryRoot();
        }

        m_gl_widget->sceneView()->sceneGraph()->resetTemporaryRoot();

        QMetaObject::invokeMethod(
            this,
            [this]() {
                m_gl_widget->updateRenderData();
                m_3DForm->clippingCtrl()->updateClipping(true);
            },
            Qt::BlockingQueuedConnection);
    }
    else {
        m_voxel_list.clear();
        m_opt2d_list.clear();
        m_opt3d_list.clear();
        m_other_list.clear();

        auto root_node = m_gl_widget->sceneView()->sceneGraph()->rootNode();
        collectTarget(root_node);

        if (m_opt2d_list.size() == 0 && (m_opt3d_list.size() * m_input.m_target_surface.size() == 0)) {
        }
        else {
            if (output_picture) {
                outputPictureOneGroup("Output Image");
            }

            if (output_text) {
                outputTextOneGroup("Output Text 2D");
            }

            if (output_1d_text) {
                outputText1dOneGroup("Output Text 1D");
            }
        }

        m_voxel_list.clear();
        m_opt2d_list.clear();
        m_opt3d_list.clear();
        m_other_list.clear();
    }

    if (opt_3d && is_suppress_opt) {
        opt_3d->suppress();
    }

    if (m_output_file_thread) {
        m_output_file_thread->stop();
        emit setProgText("Output Remaining Files...");

        bool canceled = false;
        while (!m_output_file_thread->isFinished()) {
            if (m_cancel && !canceled) {
                m_output_file_thread->cancel();
                canceled = true;
            }
            QThread::msleep(10);
        }
        m_output_file_thread->wait();

        auto& file_data_list = m_output_file_thread->fileDataList();
        for (auto& data : file_data_list) {
            QFileInfo file_info(data->path());
            m_output_folder_files[file_info.absoluteDir().absolutePath()]
                << std::make_pair(data->path(), data->isSuccess());
            if (!data->isSuccess()) {
                setFailed();
            }
        }

        delete m_output_file_thread;
        m_output_file_thread = nullptr;
    }

    QMetaObject::invokeMethod(
        this,
        [this]() {
            m_gl_widget->updateRenderData();
            m_3DForm->clippingCtrl()->updateClipping(true);
        },
        Qt::BlockingQueuedConnection);

    m_3DForm->resultCtl()->setVoxelRangeVoxels(save_range_voxels);
    m_gl_widget->suppressRender(false);
}

void ResultOutputThread::outputPictureOneGroup(const QString& label)
{
    /// GUI処理、描画処理はメインスレッドで行う必要がある（同期とって待機する）
    ///  ※ invokeMethodを使う

    auto change_colormap_minmax  = m_input.m_change_colormap_minmax;
    auto change_colormap_specify = m_input.m_change_colormap_specify;
    auto colormap_max            = m_input.m_colormap_max;

    const auto& output_folder = m_input.m_output_folder;

    auto colormap_min    = m_input.m_colormap_min;
    auto specify_range   = m_input.m_specify_range;
    auto picture_ext     = m_input.m_ext;
    auto data_size_ratio = m_input.m_data_size_ratio;
    auto pic_width       = m_input.m_pic_width;
    auto pic_height      = m_input.m_pic_height;

    auto outline_current_show = m_input.m_outline_current_show;

    const auto& opt_planes = m_input.m_target_surface;

    bool voxel_scalar_priority = false;

    for (auto& target : m_voxel_list) {
        Voxel* voxel = target.m_node->object<Voxel>();
        if (!outline_current_show) {
            voxel->setDrawShading(false);
            voxel->setDrawWireframe(true);
            target.m_node->show();
        }
        else {
            if (target.m_node->isVisible() && voxel->isDrawShading()) {
                voxel_scalar_priority = true;
            }
        }
        voxel->setTransparent(1.0f);
        voxel->setProjectionOpt(nullptr);
    }
    for (auto& target : m_opt2d_list) {
        target.m_node->hide();
    }
    for (auto& target : m_opt3d_list) {
        target.m_node->hide();
    }
    for (auto& target : m_other_list) {
        target.m_node->hide();
    }

    auto save_scene = m_gl_widget->sceneView()->saveCameraState();
    m_gl_widget->sceneView()->setProjectionType(SceneView::ProjectionType::Ortho);

    const auto save_background_color    = m_gl_widget->backgroundColor();
    const auto save_background_gradient = m_gl_widget->backGroundColorGradient();

    const auto wireframe_color       = m_gl_widget->voxelWireframeColor();
    float      wireframe_width       = m_gl_widget->voxelWireframeWidth();
    bool       wireframe_color_shape = m_gl_widget->isVoxelWireframeColorShape();

    if (!outline_current_show) {
        m_gl_widget->setVoxelWireframeColor(Point4f(0, 0, 0, 0), false);
        m_gl_widget->setVoxelWireframeWidth(1.0f / m_gl_widget->devicePixelRatio(), false);
        m_gl_widget->setVoxelWireframeColorShape(false, false);
    }
    else {
        m_gl_widget->setVoxelWireframeWidth(wireframe_width / m_gl_widget->devicePixelRatio(), false);
    }

    if (m_input.m_background_color_change) {
        m_gl_widget->setBackgroundColor(m_input.m_background_color, false);
        m_gl_widget->setBackGroundColorGradient(m_input.m_background_grad, false);
    }

    auto back_thresholds = m_gl_widget->textureDivision();

    std::vector<float> thresholds;

    if (change_colormap_specify) {
        m_3DForm->resultCtl()->calcThresholds(colormap_min, colormap_max, thresholds);
        QMetaObject::invokeMethod(
            this, [this, &thresholds]() { m_gl_widget->set3DTextureDivision(thresholds); },
            Qt::BlockingQueuedConnection);
    }

    int prog_min   = 0;
    int prog_max   = 100;
    int prog_step  = 1;
    int prog_range = (prog_max - prog_min) / prog_step + 1;

    int data_all_size = m_opt2d_list.size() + opt_planes.size() * m_opt3d_list.size();
    int prog_count    = data_all_size / prog_range + 1;
    int data_count    = 0;

    emit setProgText(label);
    emit setProgMessage(prog_min);

    // std::set<QString>          set_uniquebasename;
    // std::map<QString, QString> map_path_to_uniquebasename;
    auto& set_basename = m_set_basename_image;

    bool add_colorbar = m_input.m_output_colorbar;

    /// Op2
    for (auto& target : m_opt2d_list) {
        if (data_count % prog_count == 0) {
            int progress_value = prog_min + ((data_count / prog_count) * prog_step);
            if (progress_value > prog_max) progress_value = prog_max;
            emit setProgMessage(progress_value);
        }
        if (m_cancel) {
            break;
        }
        data_count++;

        const std::wstring& file_path = target.m_node->userAttributeString(L"File Path");
        if (file_path.empty()) {
            continue;
        }
        QString q_file_path = QString::fromStdWString(file_path);

        target.m_node->show();

        /// 子要素の表示ステータス
        std::vector<TargetNode> save_children;

        VoxelScalar* voxel = target.m_node->object<VoxelScalar>();

        RenderEditableMesh* mesh = (RenderEditableMesh*)target.m_node->renderable();

        bool        is_empty_data = false;
        const auto& vertex_list =
            mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
        if (vertex_list.empty()) {
            is_empty_data = true;
        }

        auto range_min = voxel->rangeMin();
        auto range_max = voxel->rangeMax();

        /// 範囲指定のとき元データを空にしている。
        if (is_empty_data) {
            target.m_node->clearDisplayData();
            voxel->createShapeForTexture();    /// BoundingBox用にSpecified Range適用の場合でも作る

            if (!specify_range) {
                for (auto& child_target : target.m_node->children()) {
                    TargetNode child_node;
                    child_node.m_node = child_target;
                    child_node.m_show = child_target->isShow();

                    save_children.emplace_back(child_node);
                    child_target->hide();
                }
                voxel->setRangeMinMax(std::vector<std::pair<float, float>>());
                mesh->markRenderDirty();
                // m_gl_widget->createRenderData(voxel);
            }
        }

        QFileInfo fileInfo(q_file_path);
        QString   baseName = fileInfo.completeBaseName();

        QString dirPath;
        if (output_folder.isEmpty()) {
            dirPath = fileInfo.absoluteDir().absolutePath() + QDir::separator() + QString("pics");
        }
        else {
            dirPath = output_folder + QDir::separator() + QString("pics");
        }
        QDir doc_dir(dirPath);
        if (!doc_dir.exists()) {
            doc_dir.mkpath(dirPath);
        }
        m_output_folders.insert(dirPath);

        QString create_file_path = dirPath + QDir::separator() + baseName + picture_ext;

        /// 出力フォルダ指定の場合、ファイル名重複があり得るので対策
        if (!output_folder.isEmpty()) {
            QString baseName_lower = baseName;
            baseName_lower         = baseName_lower.toLower();

            QString check_name;
            int     check_count = 1;
            while (check_count < 1000) {
                if (check_count == 1) {
                    check_name = baseName_lower;
                }
                else {
                    check_name = baseName_lower + QString("_") + QString::number(check_count);
                }
                if (set_basename.insert(check_name).second) {
                    break;
                }
                ++check_count;
            }
            if (check_count > 1) {
                create_file_path =
                    dirPath + QDir::separator() + baseName + QString("_") + QString::number(check_count) + picture_ext;
            }
        }

        m_3DForm->resultCtl()->viewOnPlaneNoChangeGUI(target.m_node.ptr());

        int width, height;
        if (data_size_ratio != 0) {
            width  = voxel->nX() * data_size_ratio;
            height = voxel->nY() * data_size_ratio;
        }
        else {
            float aspect_ratio = (float)voxel->nX() / (float)voxel->nY();
            if (pic_width / aspect_ratio <= pic_height) {
                width  = pic_width;
                height = static_cast<int>(std::round(width / aspect_ratio));
            }
            else {
                height = pic_height;
                width  = static_cast<int>(std::round(height * aspect_ratio));
            }
        }

        m_gl_widget->sceneView()->setViewPort(0, 0, width, height);

        /// 画像境界が不定なので拡張
        BoundingBox3f bbox = target.m_node->calculateBoundingBox(Matrix4x4f(), false);
        bbox.shrinkBy(-voxel->dX() * 0.01, -voxel->dY() * 0.01,
                      -voxel->dZ() * 0.01);    /// expandByが動作違う。。。shrinkByの符合で調整
        m_gl_widget->sceneView()->fitDisplayObjectBox(target.m_node->parentPathMatrix() * bbox, 1.0f);

        if (is_empty_data) {
            if (specify_range) {
                /// Specified Range適用のときはFitDisplayまで
                target.m_node->clearDisplayData();
            }
        }

        QImage image;
        if (change_colormap_minmax) {
            m_3DForm->resultCtl()->calcThresholds(voxel->minValue(), voxel->maxValue(), thresholds);
            QMetaObject::invokeMethod(
                this,
                [this, &width, &height, &image, &thresholds, voxel_scalar_priority, add_colorbar]() {
                    m_gl_widget->set3DTextureDivision(thresholds);
                    m_gl_widget->updateRenderData();
                    m_gl_widget->outputResultImage(width, height, voxel_scalar_priority, add_colorbar, image);
                },
                Qt::BlockingQueuedConnection);
        }
        else {
            QMetaObject::invokeMethod(
                this,
                [this, &width, &height, &image, voxel_scalar_priority, add_colorbar]() {
                    m_gl_widget->updateRenderData();
                    m_gl_widget->outputResultImage(width, height, voxel_scalar_priority, add_colorbar, image);
                },
                Qt::BlockingQueuedConnection);
        }
        if (data_size_ratio == 0) {
            int image_width  = image.width();
            int image_height = image.height();

            bool change = false;
            if (image_width > pic_width) {
                float ratio  = (float)pic_width / (float)image_width;
                image_width  = pic_width;
                image_height = (int)((float)ratio * (float)image_height);
                change       = true;
            }
            if (image_height > pic_height) {
                float ratio  = (float)pic_height / (float)image_height;
                image_height = pic_height;
                image_width  = (int)((float)ratio * (float)image_width);
                change       = true;
            }
            if (change) {
                image = image.scaled(image_width, image_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
        }
        if (!m_output_file_thread) {
            m_output_file_thread = new OutputFileThread();
            m_output_file_thread->start();
        }
        m_output_file_thread->addFileData(image, create_file_path);
        // image.save(create_file_path);

        if (is_empty_data) {
            if (!specify_range) {
                voxel->setRangeMinMax(range_min, range_max);
                target.m_node->clearDisplayData();
            }
        }

        /// 子要素の表示戻す。親は非表示（ほかで描画させない）
        target.m_node->hide();

        for (auto& child_target : save_children) {
            if (child_target.m_show)
                child_target.m_node->show();
            else
                child_target.m_node->hide();
        }
    }

    if (!opt_planes.empty()) {
        /// Opt - ループ内に無駄もあるが、基本１つだけなのでこれで
        for (auto& target : m_opt3d_list) {
            if (m_cancel) {
                break;
            }
            VoxelScalar* voxel = target.m_node->object<VoxelScalar>();

            RenderEditableMesh* mesh = (RenderEditableMesh*)target.m_node->renderable();

            const std::wstring& file_path = target.m_node->userAttributeString(L"File Path");
            if (file_path.empty()) {
                if (data_count % prog_count == 0) {
                    int progress_value = prog_min + ((data_count / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                if (m_cancel) {
                    break;
                }
                data_count += opt_planes.size();
                continue;
            }
            QString q_file_path = QString::fromStdWString(file_path);

            target.m_node->show();

            /// 子要素の表示ステータス
            std::vector<TargetNode> save_children;

            bool        is_empty_data = false;
            const auto& vertex_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
            if (vertex_list.empty()) {
                is_empty_data = true;
            }

            auto range_min = voxel->rangeMin();
            auto range_max = voxel->rangeMax();

            /// 範囲指定のとき元データを空にしている。
            if (is_empty_data) {
                target.m_node->clearDisplayData();
                voxel->createShapeForTexture();    /// BoundingBox用にSpecified Range適用の場合でも作る

                if (!specify_range) {
                    for (auto& child_target : target.m_node->children()) {
                        TargetNode child_node;
                        child_node.m_node = child_target;
                        child_node.m_show = child_target->isShow();

                        save_children.emplace_back(child_node);
                        child_target->hide();
                    }
                    voxel->setRangeMinMax(std::vector<std::pair<float, float>>());
                    mesh->markRenderDirty();
                    // m_gl_widget->createRenderData(voxel);
                }
            }

            QFileInfo fileInfo(q_file_path);
            QString   baseName = fileInfo.completeBaseName();

            QString dirPath;
            if (output_folder.isEmpty()) {
                dirPath = fileInfo.absoluteDir().absolutePath() + QDir::separator() + QString("pics");
            }
            else {
                dirPath = output_folder + QDir::separator() + QString("pics");
            }

            QString base_path = dirPath + QDir::separator() + baseName;

            /// 出力フォルダ指定の場合、ファイル名重複があり得るので対策
            if (!output_folder.isEmpty()) {
                QString baseName_lower = baseName;
                baseName_lower         = baseName_lower.toLower();

                QString check_name;
                int     check_count = 1;
                while (check_count < 1000) {
                    if (check_count == 1) {
                        check_name = baseName_lower;
                    }
                    else {
                        check_name = baseName_lower + QString("_") + QString::number(check_count);
                    }
                    if (set_basename.insert(check_name).second) {
                        break;
                    }
                    ++check_count;
                }
                if (check_count > 1) {
                    base_path = dirPath + QDir::separator() + baseName + QString("_") + QString::number(check_count);
                }
            }

            BoundingBox3f bbox = target.m_node->calculateBoundingBox(Matrix4x4f(), false);
            bbox.shrinkBy(-voxel->originalDX() * 0.01, -voxel->originalDY() * 0.01, -voxel->originalDZ() * 0.01);
            if (is_empty_data) {
                if (specify_range) {
                    /// Specified Range適用のときはFitDisplayまで
                    target.m_node->clearDisplayData();
                }
            }

            if (change_colormap_minmax) {
                m_3DForm->resultCtl()->calcThresholds(voxel->minValue(), voxel->maxValue(), thresholds);
                QMetaObject::invokeMethod(
                    this, [this, &thresholds]() { m_gl_widget->set3DTextureDivision(thresholds); },
                    Qt::BlockingQueuedConnection);
            }

            bool dir_check = true;

            std::set<QString> add_names;

            for (auto& plane_info : opt_planes) {
                if (data_count % prog_count == 0) {
                    int progress_value = prog_min + ((data_count / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                if (m_cancel) {
                    break;
                }
                data_count++;

                /// 2D向き
                int nx = 0;
                int ny = 0;
                /// 3D方向
                int nz = -1;

                /// セル範囲チェック
                /// ファイル名生成
                QString add_name;
                Planef  plane;
                int     dir = plane_info.m_dir;
                switch (dir) {
                    case 0:    /// xz
                    {
                        nx = voxel->originalX();
                        ny = voxel->originalZ();
                        nz = voxel->originalY();
                        plane.setFromNormalAndPoint(
                            Point3f(0, -1, 0),
                            Point3f(0, voxel->originalDY() * (float)plane_info.cell(voxel) + 0.5 * voxel->originalDY(),
                                    0));
                        add_name = QString("_Y");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDY() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    case 1:    /// yz
                    {
                        nx = voxel->originalY();
                        ny = voxel->originalZ();
                        nz = voxel->originalX();
                        plane.setFromNormalAndPoint(
                            Point3f(1, 0, 0),
                            Point3f(voxel->originalDX() * (float)plane_info.cell(voxel) + 0.5 * voxel->originalDX(), 0,
                                    0));
                        add_name = QString("_X");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDX() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    case 2:    /// xy
                    {
                        nx = voxel->originalX();
                        ny = voxel->originalY();
                        nz = voxel->originalZ();
                        plane.setFromNormalAndPoint(
                            Point3f(0, 0, -1),
                            Point3f(0, 0,
                                    voxel->originalDZ() * (float)plane_info.cell(voxel) + 0.5 * voxel->originalDZ()));
                        add_name = QString("_Z");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDZ() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    default:
                        break;
                }

                /// 範囲外
                int cell = plane_info.cell(voxel);
                if (cell >= nz || cell < 0) {
                    continue;
                }

                /// 名前がかぶる（断面位置が同じ）場合はスキップ
                if (!add_names.insert(add_name).second) {
                    continue;
                }

                if (dir_check) {
                    QDir doc_dir(dirPath);
                    if (!doc_dir.exists()) {
                        doc_dir.mkpath(dirPath);
                    }
                    m_output_folders.insert(dirPath);
                    dir_check = false;
                }

                QString create_file_path = base_path + add_name + picture_ext;

                m_3DForm->resultCtl()->viewOnPlaneNoChangeGUI(plane);

                int width, height;
                if (data_size_ratio != 0) {
                    width  = nx * data_size_ratio;
                    height = ny * data_size_ratio;
                }
                else {
                    float aspect_ratio = (float)nx / (float)ny;
                    if (pic_width / aspect_ratio <= pic_height) {
                        width  = pic_width;
                        height = static_cast<int>(std::round(width / aspect_ratio));
                    }
                    else {
                        height = pic_height;
                        width  = static_cast<int>(std::round(height * aspect_ratio));
                    }
                }
                m_gl_widget->sceneView()->setViewPort(0, 0, width, height);

                m_gl_widget->sceneView()->fitDisplayObjectBox(target.m_node->parentPathMatrix() * bbox, 1.0f);

                QImage image;

                QMetaObject::invokeMethod(
                    this,
                    [this, &width, &height, &image, voxel_scalar_priority, add_colorbar]() {
                        m_gl_widget->updateRenderData();
                        m_gl_widget->outputResultImage(width, height, voxel_scalar_priority, add_colorbar, image);
                    },
                    Qt::BlockingQueuedConnection);

                if (data_size_ratio == 0) {
                    int image_width  = image.width();
                    int image_height = image.height();

                    bool change = false;
                    if (image_width > pic_width) {
                        float ratio  = (float)pic_width / (float)image_width;
                        image_width  = pic_width;
                        image_height = (int)((float)ratio * (float)image_height);
                        change       = true;
                    }
                    if (image_height > pic_height) {
                        float ratio  = (float)pic_height / (float)image_height;
                        image_height = pic_height;
                        image_width  = (int)((float)ratio * (float)image_width);
                        change       = true;
                    }
                    if (change) {
                        image =
                            image.scaled(image_width, image_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                    }
                }
                if (!m_output_file_thread) {
                    m_output_file_thread = new OutputFileThread();
                    m_output_file_thread->start();
                }
                m_output_file_thread->addFileData(image, create_file_path);
                // image.save(create_file_path);
            }

            if (is_empty_data) {
                if (!specify_range) {
                    voxel->setRangeMinMax(range_min, range_max);
                    target.m_node->clearDisplayData();
                }
            }

            /// 子要素の表示戻す。親は非表示（ほかで描画させない）
            target.m_node->hide();

            for (auto& child_target : save_children) {
                if (child_target.m_show)
                    child_target.m_node->show();
                else
                    child_target.m_node->hide();
            }
        }
    }

    if (change_colormap_minmax || change_colormap_specify) {
        QMetaObject::invokeMethod(
            this, [this, &back_thresholds]() { m_gl_widget->set3DTextureDivision(back_thresholds); },
            Qt::BlockingQueuedConnection);
    }

    m_gl_widget->sceneView()->loadCameraState(save_scene.ptr());

    m_gl_widget->setBackgroundColor(save_background_color, false);
    m_gl_widget->setBackGroundColorGradient(save_background_gradient, false);

    m_gl_widget->setVoxelWireframeColor(wireframe_color, false);
    m_gl_widget->setVoxelWireframeWidth(wireframe_width, false);
    m_gl_widget->setVoxelWireframeColorShape(wireframe_color_shape, false);

    for (auto& target : m_voxel_list) {
        Voxel* voxel = target.m_node->object<Voxel>();
        if (!outline_current_show) {
            voxel->setDrawShading(target.m_shading);
            voxel->setDrawWireframe(target.m_wireframe);
            if (target.m_show)
                target.m_node->show();
            else
                target.m_node->hide();
        }
        voxel->setTransparent(target.m_transparent);
        voxel->setProjectionOpt(target.m_proj_opt);
    }
    for (auto& target : m_opt2d_list) {
        if (target.m_show)
            target.m_node->show();
        else
            target.m_node->hide();
    }
    for (auto& target : m_opt3d_list) {
        if (target.m_show)
            target.m_node->show();
        else
            target.m_node->hide();
    }
    for (auto& target : m_other_list) {
        if (target.m_show)
            target.m_node->show();
        else
            target.m_node->hide();
    }
}

void ResultOutputThread::outputTextOneGroup(const QString& label)
{
    const auto& output_folder = m_input.m_output_folder;
    const auto& opt_planes    = m_input.m_target_surface;

    QString txt_ext = ".csv";

    int prog_min   = 0;
    int prog_max   = 100;
    int prog_step  = 1;
    int prog_range = (prog_max - prog_min) / prog_step + 1;

    int data_all_size = m_opt2d_list.size() + opt_planes.size() * m_opt3d_list.size();
    int prog_count    = data_all_size / prog_range + 1;
    int data_count    = 0;

    emit setProgText(label);
    emit setProgMessage(prog_min);

    auto& set_basename = m_set_basename_text;

    /// Op2
    for (auto& target : m_opt2d_list) {
        if (data_count % prog_count == 0) {
            int progress_value = prog_min + ((data_count / prog_count) * prog_step);
            if (progress_value > prog_max) progress_value = prog_max;
            emit setProgMessage(progress_value);
        }
        if (m_cancel) {
            break;
        }
        data_count++;

        const std::wstring& file_path = target.m_node->userAttributeString(L"File Path");
        if (file_path.empty()) {
            continue;
        }
        QString q_file_path = QString::fromStdWString(file_path);

        VoxelScalar* voxel = target.m_node->object<VoxelScalar>();

        QFileInfo fileInfo(q_file_path);
        QString   baseName = fileInfo.completeBaseName();

        QString dirPath;
        if (output_folder.isEmpty()) {
            dirPath = fileInfo.absoluteDir().absolutePath() + QDir::separator() + QString("text");
        }
        else {
            dirPath = output_folder + QDir::separator() + QString("text");
        }
        QDir doc_dir(dirPath);
        if (!doc_dir.exists()) {
            doc_dir.mkpath(dirPath);
        }
        m_output_folders.insert(dirPath);

        QString create_file_path = dirPath + QDir::separator() + baseName + txt_ext;

        /// 出力フォルダ指定の場合、ファイル名重複があり得るので対策
        if (!output_folder.isEmpty()) {
            QString baseName_lower = baseName;
            baseName_lower         = baseName_lower.toLower();

            QString check_name;
            int     check_count = 1;
            while (check_count < 1000) {
                if (check_count == 1) {
                    check_name = baseName_lower;
                }
                else {
                    check_name = baseName_lower + QString("_") + QString::number(check_count);
                }
                if (set_basename.insert(check_name).second) {
                    break;
                }
                ++check_count;
            }
            if (check_count > 1) {
                create_file_path =
                    dirPath + QDir::separator() + baseName + QString("_") + QString::number(check_count) + txt_ext;
            }
        }

        int         section_dir = -1;    /// 0:xz, 1:yz, 2:xy, 3:dz
        const auto& section     = target.m_node->userAttributeString(L"Section");
        if (section.compare(L"Y") == 0) {
            section_dir = 0;
        }
        else if (section.compare(L"X") == 0) {
            section_dir = 1;
        }
        else if (section.compare(L"Z") == 0) {
            section_dir = 2;
        }
        else if (section.compare(L"D45") == 0 || section.compare(L"D135") == 0) {
            section_dir = 3;
        }
        else {
            assert(0);
        }

        /// TEXT出力
        output2DText(voxel, section_dir, create_file_path);
    }

    if (!opt_planes.empty()) {
        /// Opt - ループ内に無駄もあるが、基本１つだけなのでこれで
        for (auto& target : m_opt3d_list) {
            if (m_cancel) {
                break;
            }
            VoxelScalar* voxel = target.m_node->object<VoxelScalar>();

            const std::wstring& file_path = target.m_node->userAttributeString(L"File Path");
            if (file_path.empty()) {
                if (data_count % prog_count == 0) {
                    int progress_value = prog_min + ((data_count / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                if (m_cancel) {
                    break;
                }
                data_count += opt_planes.size();
                continue;
            }
            QString q_file_path = QString::fromStdWString(file_path);

            QFileInfo fileInfo(q_file_path);
            QString   baseName = fileInfo.completeBaseName();

            QString dirPath;
            if (output_folder.isEmpty()) {
                dirPath = fileInfo.absoluteDir().absolutePath() + QDir::separator() + QString("text");
            }
            else {
                dirPath = output_folder + QDir::separator() + QString("text");
            }

            QString base_path = dirPath + QDir::separator() + baseName;

            /// 出力フォルダ指定の場合、ファイル名重複があり得るので対策
            if (!output_folder.isEmpty()) {
                QString baseName_lower = baseName;
                baseName_lower         = baseName_lower.toLower();

                QString check_name;
                int     check_count = 1;
                while (check_count < 1000) {
                    if (check_count == 1) {
                        check_name = baseName_lower;
                    }
                    else {
                        check_name = baseName_lower + QString("_") + QString::number(check_count);
                    }
                    if (set_basename.insert(check_name).second) {
                        break;
                    }
                    ++check_count;
                }
                if (check_count > 1) {
                    base_path = dirPath + QDir::separator() + baseName + QString("_") + QString::number(check_count);
                }
            }

            bool dir_check = true;

            std::set<QString> add_names;

            for (auto& plane_info : opt_planes) {
                if (data_count % prog_count == 0) {
                    int progress_value = prog_min + ((data_count / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                if (m_cancel) {
                    break;
                }
                data_count++;

                /// 3D方向
                int nz = -1;

                /// セル範囲チェック
                /// ファイル名生成
                QString add_name;
                int     dir = plane_info.m_dir;
                switch (dir) {
                    case 0:    /// xz
                    {
                        nz       = voxel->originalY();
                        add_name = QString("_Y");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDY() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    case 1:    /// yz
                    {
                        nz       = voxel->originalX();
                        add_name = QString("_X");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDX() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    case 2:    /// xy
                    {
                        nz       = voxel->originalZ();
                        add_name = QString("_Z");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDZ() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    default:
                        break;
                }

                /// 範囲外
                int cell = plane_info.cell(voxel);
                if (cell >= nz || cell < 0) {
                    continue;
                }

                /// 名前がかぶる（断面位置が同じ）場合はスキップ
                if (!add_names.insert(add_name).second) {
                    continue;
                }

                QString create_file_path = base_path + add_name + txt_ext;

                if (dir_check) {
                    QDir doc_dir(dirPath);
                    if (!doc_dir.exists()) {
                        doc_dir.mkpath(dirPath);
                    }
                    dir_check = false;
                    m_output_folders.insert(dirPath);
                }

                /// TEXT出力
                output2DText(voxel, plane_info, create_file_path);
            }
        }
    }
}

void ResultOutputThread::outputText1dOneGroup(const QString& label)
{
    const auto& output_folder = m_input.m_output_folder;
    const auto& opt_planes    = m_input.m_target_surface;

    int prog_min   = 0;
    int prog_max   = 100;
    int prog_step  = 1;
    int prog_range = (prog_max - prog_min) / prog_step + 1;

    int data_all_size = m_opt2d_list.size() + opt_planes.size() * m_opt3d_list.size();
    int prog_count    = data_all_size / prog_range + 1;
    int data_count    = 0;

    emit setProgText(label);
    emit setProgMessage(prog_min);

    auto& set_basename = m_set_basename_text1d;

    /// Opt 2d
    for (auto& target : m_opt2d_list) {
        if (data_count % prog_count == 0) {
            int progress_value = prog_min + ((data_count / prog_count) * prog_step);
            if (progress_value > prog_max) progress_value = prog_max;
            emit setProgMessage(progress_value);
        }
        if (m_cancel) {
            break;
        }
        data_count++;

        const std::wstring& file_path = target.m_node->userAttributeString(L"File Path");
        if (file_path.empty()) {
            continue;
        }

        int         section_dir = -1;    /// 0:xz, 1:yz, 2:xy, 3:dz
        const auto& section     = target.m_node->userAttributeString(L"Section");
        if (section.compare(L"Y") == 0) {
            section_dir = 0;
        }
        else if (section.compare(L"X") == 0) {
            section_dir = 1;
        }
        else if (section.compare(L"Z") == 0) {
            section_dir = 2;
        }
        else if (section.compare(L"D45") == 0 || section.compare(L"D135") == 0) {
            section_dir = 3;
        }
        else {
            assert(0);
            continue;
        }

        QString q_file_path = QString::fromStdWString(file_path);

        VoxelScalar* voxel = target.m_node->object<VoxelScalar>();

        QFileInfo fileInfo(q_file_path);
        QString   baseName = fileInfo.completeBaseName();

        QString dirPath;
        if (output_folder.isEmpty()) {
            dirPath = fileInfo.absoluteDir().absolutePath() + QDir::separator() + QString("text1d");
        }
        else {
            dirPath = output_folder + QDir::separator() + QString("text1d");
        }

        QString create_file_path = dirPath + QDir::separator() + baseName;

        /// 出力フォルダ指定の場合、ファイル名重複があり得るので対策
        if (!output_folder.isEmpty()) {
            QString baseName_lower = baseName;
            baseName_lower         = baseName_lower.toLower();

            QString check_name;
            int     check_count = 1;
            while (check_count < 1000) {
                if (check_count == 1) {
                    check_name = baseName_lower;
                }
                else {
                    check_name = baseName_lower + QString("_") + QString::number(check_count);
                }
                if (set_basename.insert(check_name).second) {
                    break;
                }
                ++check_count;
            }
            if (check_count > 1) {
                create_file_path = dirPath + QDir::separator() + baseName + QString("_") + QString::number(check_count);
            }
        }

        /// TEXT出力
        output1DText(voxel, section_dir, dirPath, create_file_path);
    }

    if (!opt_planes.empty()) {
        /// Opt - ループ内に無駄もあるが、基本１つだけなのでこれで
        for (auto& target : m_opt3d_list) {
            if (m_cancel) {
                break;
            }
            VoxelScalar* voxel = target.m_node->object<VoxelScalar>();

            const std::wstring& file_path = target.m_node->userAttributeString(L"File Path");
            if (file_path.empty()) {
                if (data_count % prog_count == 0) {
                    int progress_value = prog_min + ((data_count / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                if (m_cancel) {
                    break;
                }
                data_count += opt_planes.size();
                continue;
            }
            QString q_file_path = QString::fromStdWString(file_path);

            QFileInfo fileInfo(q_file_path);
            QString   baseName = fileInfo.completeBaseName();

            QString dirPath;
            if (output_folder.isEmpty()) {
                dirPath = fileInfo.absoluteDir().absolutePath() + QDir::separator() + QString("text1d");
            }
            else {
                dirPath = output_folder + QDir::separator() + QString("text1d");
            }

            QString base_path = dirPath + QDir::separator() + baseName;

            /// 出力フォルダ指定の場合、ファイル名重複があり得るので対策
            if (!output_folder.isEmpty()) {
                QString baseName_lower = baseName;
                baseName_lower         = baseName_lower.toLower();

                QString check_name;
                int     check_count = 1;
                while (check_count < 1000) {
                    if (check_count == 1) {
                        check_name = baseName_lower;
                    }
                    else {
                        check_name = baseName_lower + QString("_") + QString::number(check_count);
                    }
                    if (set_basename.insert(check_name).second) {
                        break;
                    }
                    ++check_count;
                }
                if (check_count > 1) {
                    base_path = dirPath + QDir::separator() + baseName + QString("_") + QString::number(check_count);
                }
            }

            std::set<QString> add_names;

            for (auto& plane_info : opt_planes) {
                if (data_count % prog_count == 0) {
                    int progress_value = prog_min + ((data_count / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                if (m_cancel) {
                    break;
                }
                data_count++;

                /// 3D方向
                int nz = -1;

                /// セル範囲チェック
                /// ファイル名生成
                QString add_name;
                int     dir = plane_info.m_dir;
                switch (dir) {
                    case 0:    /// xz
                    {
                        nz       = voxel->originalY();
                        add_name = QString("_Y");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDY() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    case 1:    /// yz
                    {
                        nz       = voxel->originalX();
                        add_name = QString("_X");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDX() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    case 2:    /// xy
                    {
                        nz       = voxel->originalZ();
                        add_name = QString("_Z");
                        add_name +=
                            QString::number((int)std::round(voxel->originalDZ() * (float)plane_info.cell(voxel) * 1e3f))
                                .rightJustified(5, '0');
                    } break;
                    default:
                        break;
                }

                /// 範囲外
                if (plane_info.cell(voxel) >= nz) {
                    continue;
                }

                /// 名前がかぶる（断面位置が同じ）場合はスキップ
                if (!add_names.insert(add_name).second) {
                    continue;
                }

                QString create_file_path = base_path + add_name;

                /// TEXT出力
                output1DText(voxel, plane_info, dirPath, create_file_path);
            }
        }
    }
}

void ResultOutputThread::output2DText(VoxelScalar* voxel, int dir, const QString& file_path)
{
    const auto data = voxel->scalarData();
    if (!data) {
        return;
    }

    if (!voxel->is2DTexture()) {
        return;
    }

    int nX = voxel->nX();
    int nY = voxel->nY();

    float mesh_size_x = voxel->dX() * 1e3f;    /// meshsize (um単位)
    float mesh_size_y = voxel->dY() * 1e3f;    /// meshsize (um単位)
    int   decimals_x  = ResultOutputDlg::valueDecimals(mesh_size_x);
    int   decimals_y  = ResultOutputDlg::valueDecimals(mesh_size_y);

    m_buffer.clear();

    switch (dir) {
        case 0:    /// xz
            m_buffer += "z\\x,";
            break;
        case 1:    /// yz
            m_buffer += "z\\y,";
            break;
        case 2:    /// xy
            m_buffer += "y\\x,";
            break;
        case 3:    /// d
            m_buffer += "z\\d,";
            break;
        default:
            m_buffer += "y\\x,";
            break;
    }

    for (int ic = 0; ic < nX; ++ic) {
        m_buffer += QString("%1").arg(((float)ic + 0.5) * (float)mesh_size_x, 1, 'f', decimals_x);
        if (ic < nX - 1) {
            m_buffer += ",";
        }
    }
    m_buffer += "\n";

    for (int jc = 0; jc < nY; ++jc) {
        m_buffer += QString("%1").arg(((float)jc + 0.5) * (float)mesh_size_y, 1, 'f', decimals_y);
        m_buffer += ",";
        for (int ic = 0; ic < nX; ++ic) {
            m_buffer += QString::number(data[(__int64)(jc) * (__int64)(nX) + (__int64)ic]);
            if (ic < nX - 1) {
                m_buffer += ",";
            }
        }
        m_buffer += "\n";
    }

    if (!m_output_file_thread) {
        m_output_file_thread = new OutputFileThread();
        m_output_file_thread->start();
    }
    m_output_file_thread->addFileData(m_buffer, file_path);
    // QFile out_file(file_path);
    // if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //     QTextStream ts(&out_file);
    //     ts << m_buffer;
    //     out_file.close();
    // }
}

void ResultOutputThread::output2DText(VoxelScalar* voxel, const ResultOutputDlg::PlaneInfo& plane,
                                      const QString& file_path)
{
    const auto data = voxel->scalarData();
    if (!data) {
        return;
    }

    if (voxel->is2DTexture()) {
        return;
    }

    int nX = voxel->nX();
    int nY = voxel->nY();
    int nZ = voxel->nZ();

    __int64 NyNz = (__int64)nY * (__int64)nZ;

    //__int64 index_ =
    //    (__int64)ix * ((__int64)nY * (__int64)nZ) + (__int64)iy * (__int64)nZ + (__int64)iz;

    if (plane.m_dir == 0) {    /// xz

        float mesh_size_x = voxel->dX() * 1e3f;    /// meshsize (um単位)
        float mesh_size_z = voxel->dZ() * 1e3f;    /// meshsize (um単位)

        int decimals_x = ResultOutputDlg::valueDecimals(mesh_size_x);
        int decimals_z = ResultOutputDlg::valueDecimals(mesh_size_z);

        __int64 cell = plane.cellConsiderCompress(voxel);
        if (cell >= nY || cell < 0) {
            assert(0);
            return;
        }

        m_buffer.clear();

        m_buffer += "z\\x,";
        for (int ix = 0; ix < nX; ++ix) {
            m_buffer += QString("%1").arg(((float)ix + 0.5) * (float)mesh_size_x, 1, 'f', decimals_x);
            if (ix < nX - 1) {
                m_buffer += ",";
            }
        }
        m_buffer += "\n";

        __int64 cell_index = (__int64)cell * (__int64)nZ;

        for (int iz = 0; iz < nZ; ++iz) {
            m_buffer += QString("%1").arg(((float)iz + 0.5) * (float)mesh_size_z, 1, 'f', decimals_z);
            m_buffer += ",";
            __int64 index_y = cell_index + (__int64)iz;
            for (int ix = 0; ix < nX; ++ix) {
                __int64 index = (__int64)ix * NyNz + index_y;
                m_buffer += QString::number(data[index]);
                if (ix < nX - 1) {
                    m_buffer += ",";
                }
            }
            m_buffer += "\n";
        }
    }
    else if (plane.m_dir == 1) {    /// yz

        float mesh_size_y = voxel->dY() * 1e3f;    /// meshsize (um単位)
        float mesh_size_z = voxel->dZ() * 1e3f;    /// meshsize (um単位)

        int decimals_y = ResultOutputDlg::valueDecimals(mesh_size_y);
        int decimals_z = ResultOutputDlg::valueDecimals(mesh_size_z);

        __int64 cell = plane.cellConsiderCompress(voxel);
        if (cell >= nX || cell < 0) {
            assert(0);
            return;
        }

        m_buffer.clear();

        m_buffer += "z\\y,";
        for (int iy = 0; iy < nY; ++iy) {
            m_buffer += QString("%1").arg(((float)iy + 0.5) * (float)mesh_size_y, 1, 'f', decimals_y);
            if (iy < nY - 1) {
                m_buffer += ",";
            }
        }
        m_buffer += "\n";

        __int64 cell_index = (__int64)cell * (__int64)NyNz;

        for (int iz = 0; iz < nZ; ++iz) {
            m_buffer += QString("%1").arg(((float)iz + 0.5) * (float)mesh_size_z, 1, 'f', decimals_z);
            m_buffer += ",";
            __int64 index_z = cell_index + (__int64)iz;
            for (int iy = 0; iy < nY; ++iy) {
                __int64 index = (__int64)iy * nZ + index_z;
                m_buffer += QString::number(data[index]);
                if (iy < nY - 1) {
                    m_buffer += ",";
                }
            }
            m_buffer += "\n";
        }
    }
    else if (plane.m_dir == 2) {    /// xy

        float mesh_size_x = voxel->dX() * 1e3f;    /// meshsize (um単位)
        float mesh_size_y = voxel->dY() * 1e3f;    /// meshsize (um単位)

        int decimals_x = ResultOutputDlg::valueDecimals(mesh_size_x);
        int decimals_y = ResultOutputDlg::valueDecimals(mesh_size_y);

        __int64 cell = plane.cellConsiderCompress(voxel);
        if (cell >= nZ || cell < 0) {
            assert(0);
            return;
        }

        m_buffer.clear();

        m_buffer += "y\\x,";
        for (int ix = 0; ix < nX; ++ix) {
            m_buffer += QString("%1").arg(((float)ix + 0.5) * (float)mesh_size_x, 1, 'f', decimals_x);
            if (ix < nX - 1) {
                m_buffer += ",";
            }
        }
        m_buffer += "\n";

        __int64 cell_index = (__int64)cell;

        for (int iy = 0; iy < nY; ++iy) {
            m_buffer += QString("%1").arg(((float)iy + 0.5) * (float)mesh_size_y, 1, 'f', decimals_y);
            m_buffer += ",";
            __int64 index_y = cell_index + (__int64)iy * (__int64)nZ;
            for (int ix = 0; ix < nX; ++ix) {
                __int64 index = (__int64)ix * NyNz + index_y;
                m_buffer += QString::number(data[index]);
                if (ix < nX - 1) {
                    m_buffer += ",";
                }
            }
            m_buffer += "\n";
        }
    }

    if (!m_output_file_thread) {
        m_output_file_thread = new OutputFileThread();
        m_output_file_thread->start();
    }
    m_output_file_thread->addFileData(m_buffer, file_path);
    // QFile out_file(file_path);
    // if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //     QTextStream ts(&out_file);
    //     ts << m_buffer;
    //     out_file.close();
    // }
}

void ResultOutputThread::output1DText(VoxelScalar* voxel, int dir, const QString& directory_path,
                                      const QString& file_base_path)
{
    const auto data = voxel->scalarData();
    if (!data) {
        return;
    }

    if (!voxel->is2DTexture()) {
        return;
    }

    const std::vector<ResultOutputDlg::OneDirecitonInfo>& target_list = m_input.m_target_one_direction[dir];
    if (target_list.empty()) {
        return;
    }

    int nX = voxel->nX();
    int nY = voxel->nY();

    float mesh_size_x = voxel->dX() * 1e3f;
    float mesh_size_y = voxel->dY() * 1e3f;

    int decimals_x = ResultOutputDlg::valueDecimals(mesh_size_x);
    int decimals_y = ResultOutputDlg::valueDecimals(mesh_size_y);

    bool dir_check = true;

    std::set<QString> add_names;

    for (const auto& target : target_list) {
        int cell = target.cell(voxel);

        QString add_name = target.name(voxel);

        /// 名前がかぶる（取得位置が同じ）場合はスキップ
        if (!add_names.insert(add_name).second) {
            continue;
        }

        QString file_path = file_base_path + add_name + ".csv";

        m_buffer.clear();

        /// 高さ
        if (target.m_height) {
            if (cell >= nX || cell < 0) {
                continue;
            }

            for (int ic = nY - 1; ic >= 0; --ic) {
                m_buffer += QString("%1,").arg(((float)ic + 0.5) * (float)mesh_size_y, 1, 'f', decimals_y);
                m_buffer += QString::number(data[(__int64)(ic) * (__int64)(nX) + (__int64)cell]);
                m_buffer += "\n";
            }
        }
        else {
            if (cell >= nY || cell < 0) {
                continue;
            }

            for (int ic = 0; ic < nX; ++ic) {
                m_buffer += QString("%1,").arg(((float)ic + 0.5) * (float)mesh_size_x, 1, 'f', decimals_x);
                m_buffer += QString::number(data[(__int64)(cell) * (__int64)(nX) + (__int64)ic]);
                m_buffer += "\n";
            }
        }

        if (dir_check) {
            QDir doc_dir(directory_path);
            if (!doc_dir.exists()) {
                doc_dir.mkpath(directory_path);
            }
            dir_check = false;

            m_output_folders.insert(directory_path);
        }

        if (!m_output_file_thread) {
            m_output_file_thread = new OutputFileThread();
            m_output_file_thread->start();
        }
        m_output_file_thread->addFileData(m_buffer, file_path);
        // QFile out_file(file_path);
        // if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //     QTextStream ts(&out_file);
        //     ts << m_buffer;
        //     out_file.close();
        // }
    }
}

void ResultOutputThread::output1DText(VoxelScalar* voxel, const ResultOutputDlg::PlaneInfo& plane,
                                      const QString& directory_path, const QString& file_base_path)
{
    const auto data = voxel->scalarData();
    if (!data) {
        return;
    }

    if (voxel->is2DTexture()) {
        return;
    }

    const std::vector<ResultOutputDlg::OneDirecitonInfo>& target_list = m_input.m_target_one_direction[plane.m_dir];
    if (target_list.empty()) {
        return;
    }

    int nX = voxel->nX();
    int nY = voxel->nY();
    int nZ = voxel->nZ();

    __int64 NyNz = (__int64)nY * (__int64)nZ;

    //__int64 index_ =
    //    (__int64)ix * ((__int64)nY * (__int64)nZ) + (__int64)iy * (__int64)nZ + (__int64)iz;

    bool dir_check = true;

    std::set<QString> add_names;

    if (plane.m_dir == 0) {    /// xz

        float mesh_size_x = voxel->dX() * 1e3f;    /// meshsize (um単位)
        float mesh_size_z = voxel->dZ() * 1e3f;    /// meshsize (um単位)

        int decimals_x = ResultOutputDlg::valueDecimals(mesh_size_x);
        int decimals_z = ResultOutputDlg::valueDecimals(mesh_size_z);

        m_buffer.clear();

        __int64 plane_cell = (__int64)plane.cellConsiderCompress(voxel);
        if (plane_cell >= nY || plane_cell < 0) {
            assert(0);
            return;
        }

        __int64 cell_index = (__int64)plane_cell * (__int64)nZ;

        for (const auto& target : target_list) {
            __int64 cell = target.cellConsiderCompress(voxel);

            QString add_name = target.name(voxel);

            /// 名前がかぶる（取得位置が同じ）場合はスキップ
            if (!add_names.insert(add_name).second) {
                continue;
            }

            QString file_path = file_base_path + add_name + ".csv";

            m_buffer.clear();

            /// 高さ
            if (target.m_height) {
                if (cell >= nX || cell < 0) {
                    continue;
                }
                for (int iz = nZ - 1; iz >= 0; --iz) {
                    __int64 index = cell * NyNz + cell_index + (__int64)iz;

                    m_buffer += QString("%1,").arg(((float)iz + 0.5) * (float)mesh_size_z, 1, 'f', decimals_z);
                    m_buffer += QString::number(data[index]);
                    m_buffer += "\n";
                }
            }
            else {
                if (cell >= nZ || cell < 0) {
                    continue;
                }
                for (int ix = 0; ix < nX; ++ix) {
                    __int64 index = (__int64)ix * NyNz + cell_index + cell;

                    m_buffer += QString("%1,").arg(((float)ix + 0.5) * (float)mesh_size_x, 1, 'f', decimals_x);
                    m_buffer += QString::number(data[index]);
                    m_buffer += "\n";
                }
            }

            if (dir_check) {
                QDir doc_dir(directory_path);
                if (!doc_dir.exists()) {
                    doc_dir.mkpath(directory_path);
                }
                m_output_folders.insert(directory_path);
                dir_check = false;
            }

            if (!m_output_file_thread) {
                m_output_file_thread = new OutputFileThread();
                m_output_file_thread->start();
            }
            m_output_file_thread->addFileData(m_buffer, file_path);
            // QFile out_file(file_path);
            // if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            //     QTextStream ts(&out_file);
            //     ts << m_buffer;
            //     out_file.close();
            // }
        }
    }
    else if (plane.m_dir == 1) {                   /// yz
        float mesh_size_y = voxel->dY() * 1e3f;    /// meshsize (um単位)
        float mesh_size_z = voxel->dZ() * 1e3f;    /// meshsize (um単位)

        int decimals_y = ResultOutputDlg::valueDecimals(mesh_size_y);
        int decimals_z = ResultOutputDlg::valueDecimals(mesh_size_z);

        m_buffer.clear();

        __int64 plane_cell = (__int64)plane.cellConsiderCompress(voxel);
        if (plane_cell >= nX || plane_cell < 0) {
            assert(0);
            return;
        }

        __int64 cell_index = (__int64)plane_cell * (__int64)NyNz;

        for (const auto& target : target_list) {
            __int64 cell = target.cellConsiderCompress(voxel);

            QString add_name = target.name(voxel);

            /// 名前がかぶる（取得位置が同じ）場合はスキップ
            if (!add_names.insert(add_name).second) {
                continue;
            }

            QString file_path = file_base_path + add_name + ".csv";

            m_buffer.clear();

            /// 高さ
            if (target.m_height) {
                if (cell >= nY || cell < 0) {
                    continue;
                }
                for (int iz = nZ - 1; iz >= 0; --iz) {
                    __int64 index = cell_index + cell * nZ + (__int64)iz;

                    m_buffer += QString("%1,").arg(((float)iz + 0.5) * (float)mesh_size_z, 1, 'f', decimals_z);
                    m_buffer += QString::number(data[index]);
                    m_buffer += "\n";
                }
            }
            else {
                if (cell >= nZ || cell < 0) {
                    continue;
                }
                for (int iy = 0; iy < nY; ++iy) {
                    __int64 index = cell_index + (__int64)iy * nZ + cell;

                    m_buffer += QString("%1,").arg(((float)iy + 0.5) * (float)mesh_size_y, 1, 'f', decimals_y);
                    m_buffer += QString::number(data[index]);
                    m_buffer += "\n";
                }
            }

            if (dir_check) {
                QDir doc_dir(directory_path);
                if (!doc_dir.exists()) {
                    doc_dir.mkpath(directory_path);
                }
                m_output_folders.insert(directory_path);
                dir_check = false;
            }

            if (!m_output_file_thread) {
                m_output_file_thread = new OutputFileThread();
                m_output_file_thread->start();
            }
            m_output_file_thread->addFileData(m_buffer, file_path);
            // QFile out_file(file_path);
            // if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            //     QTextStream ts(&out_file);
            //     ts << m_buffer;
            //     out_file.close();
            // }
        }
    }
    else if (plane.m_dir == 2) {                   /// xy
        float mesh_size_x = voxel->dX() * 1e3f;    /// meshsize (um単位)
        float mesh_size_y = voxel->dY() * 1e3f;    /// meshsize (um単位)

        int decimals_x = ResultOutputDlg::valueDecimals(mesh_size_x);
        int decimals_y = ResultOutputDlg::valueDecimals(mesh_size_y);

        m_buffer.clear();

        __int64 plane_cell = (__int64)plane.cellConsiderCompress(voxel);
        if (plane_cell >= nZ || plane_cell < 0) {
            assert(0);
            return;
        }
        __int64 cell_index = (__int64)plane_cell;

        for (const auto& target : target_list) {
            __int64 cell = target.cellConsiderCompress(voxel);

            QString add_name = target.name(voxel);

            /// 名前がかぶる（取得位置が同じ）場合はスキップ
            if (!add_names.insert(add_name).second) {
                continue;
            }

            QString file_path = file_base_path + add_name + ".csv";

            m_buffer.clear();

            /// 高さ
            if (target.m_height) {
                if (cell >= nX || cell < 0) {
                    continue;
                }

                for (int iy = nY - 1; iy >= 0; --iy) {
                    __int64 index = cell * NyNz + (__int64)iy * nZ + cell_index;

                    m_buffer += QString("%1,").arg(((float)iy + 0.5) * (float)mesh_size_y, 1, 'f', decimals_y);
                    m_buffer += QString::number(data[index]);
                    m_buffer += "\n";
                }
            }
            else {
                if (cell >= nY || cell < 0) {
                    continue;
                }

                for (int ix = 0; ix < nX; ++ix) {
                    __int64 index = (__int64)ix * NyNz + cell * nZ + cell_index;

                    m_buffer += QString("%1,").arg(((float)ix + 0.5) * (float)mesh_size_x, 1, 'f', decimals_x);
                    m_buffer += QString::number(data[index]);
                    m_buffer += "\n";
                }
            }

            if (dir_check) {
                QDir doc_dir(directory_path);
                if (!doc_dir.exists()) {
                    doc_dir.mkpath(directory_path);
                }
                m_output_folders.insert(directory_path);
                dir_check = false;
            }

            if (!m_output_file_thread) {
                m_output_file_thread = new OutputFileThread();
                m_output_file_thread->start();
            }
            m_output_file_thread->addFileData(m_buffer, file_path);
            // QFile out_file(file_path);
            // if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            //     QTextStream ts(&out_file);
            //     ts << m_buffer;
            //     out_file.close();
            // }
        }
    }
}
