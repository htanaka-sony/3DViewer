#include "VoxOutputDlg.h"
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
#include <QtConcurrent/QtConcurrent>

#include "ui_VoxOutputDlg.h"

#define VOX_OUTPUT_TARGET_LIST_TARGET 0
#define VOX_OUTPUT_TARGET_LIST_FILENAME 1
#define VOX_OUTPUT_TARGET_LIST_TYPE 2
#define VOX_OUTPUT_TARGET_LIST_FOLDER 3
#define VOX_OUTPUT_TARGET_LIST_RELATED_FILE 4

#define VOX_OUTPUT_TARGET_LIST_FILE_FULLPATH VOX_OUTPUT_TARGET_LIST_FILENAME

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

class FolderDialog : public QDialog {
public:
    FolderDialog(const QString& labelText, const QStringList& folderPaths, QWidget* parent = nullptr) : QDialog(parent)
    {
        if (parent) {
            setWindowTitle(parent->windowTitle());
        }
        QVBoxLayout* layout = new QVBoxLayout(this);

        /// ラベル追加
        QLabel* label = new QLabel(labelText, this);
        layout->addWidget(label);

        /// スクロールエリア
        QScrollArea* scrollArea      = new QScrollArea(this);
        QWidget*     container       = new QWidget;
        QVBoxLayout* containerLayout = new QVBoxLayout(container);

        /// ハイパーリンクラベルを大量に追加
        for (const QString& path : folderPaths) {
            QLabel* linkLabel = new QLabel(QString("<a href=\"%1\">%2</a>").arg(path, path), container);
            linkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            linkLabel->setOpenExternalLinks(false);    // 自分で処理する
            connect(linkLabel, &QLabel::linkActivated,
                    [=](const QString& link) { QDesktopServices::openUrl(QUrl::fromLocalFile(link)); });
            containerLayout->addWidget(linkLabel);
        }
        containerLayout->addStretch();
        container->setLayout(containerLayout);

        scrollArea->setWidget(container);
        scrollArea->setWidgetResizable(true);
        layout->addWidget(scrollArea);

        /// Closeボタン
        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttonBox, &QDialogButtonBox::clicked, this, &QDialog::reject);
        layout->addWidget(buttonBox);
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

// ファイル検索関数（別スレッドで実行）
QStringList findFiles(const QString& dir, std::atomic<bool>& canceled)
{
    QStringList  files;
    QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (canceled) break;
        QString subFilePath = it.next();
        QString ext         = QFileInfo(subFilePath).suffix().toLower();
        if (ext == "vox" || ext == "fdtd") {
            files << subFilePath;
        }
    }
    return files;
}

QStringList VoxOutputDlg::searchFilesWithProgress(const QString& dir)
{
    std::atomic<bool> canceled(false);

    QProgressDialog progress("ファイル検索中...", "キャンセル", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    // progress.setMinimumDuration(1000);    /// 1秒後に表示

    QFuture<QStringList>        future = QtConcurrent::run(findFiles, dir, std::ref(canceled));
    QFutureWatcher<QStringList> watcher;
    watcher.setFuture(future);

    QEventLoop loop;

    QObject::connect(&progress, &QProgressDialog::canceled, [&]() { canceled = true; });

    QObject::connect(&watcher, &QFutureWatcher<QStringList>::finished, [&]() {
        progress.close();
        loop.quit();    /// イベントループ終了
    });

    QTimer* timer = new QTimer(&progress);    /// progressが消えればtimerも消える
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, &progress, &QProgressDialog::show);
    timer->start(1000);    /// １秒経過まで表示しない

    loop.exec();    /// ここで完了まで待機

    return watcher.result();
}

VoxOutputDlg::VoxOutputDlg(Vox3DForm* parent, MyOpenGLWidget* gl_widget)
    : m_3DForm(parent)
    , m_gl_widget(gl_widget)
    , QDialog(parent)
    , ui(new Ui::VoxOutputDlg)
{
    ui->setupUi(this);

    ui->tabWidget_base->setCustomTabBar();

    ui->radioButton_target_current->setChecked(true);
    // ui->lineEdit_target_folder->setEnabled(false);
    connect(ui->radioButton_target_current, &QRadioButton::toggled, this, [this]() { updateEnableTargetList(); });
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

            /*
            QStringList files;

            QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString subFilePath = it.next();

                QString ext = QFileInfo(subFilePath).suffix().toLower();
                if (ext == "vox") {
                    files << subFilePath;
                }
                else if (ext == "fdtd") {
                    files << subFilePath;
                }
            }
            addTargetList(files);
            */
            addTargetList(searchFilesWithProgress(dir));
        }
    });
    connect(ui->pushButton_target_import_files, &QPushButton::clicked, this, [this]() {
        QString filter = tr("All Files (*.vox *.fdtd);;vox file (*.vox);;fdtd file (*.fdtd)");

        QStringList loadfileNames = QFileDialog::getOpenFileNames(this, tr("Select file"), "", filter);

        if (!loadfileNames.isEmpty()) {
            if (!ui->radioButton_target_folder->isChecked()) {
                ui->radioButton_target_folder->setChecked(true);
            }

            addTargetList(loadfileNames);
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

    connect(ui->pushButton_output2d_picture, &QPushButton::clicked, this, [this]() { outputPicture(); });

    ui->pushButton_background_color->setEnabled(ui->checkBox_change_background->isChecked());
    ui->checkBox_background_grad->setEnabled(ui->checkBox_change_background->isChecked());

    connect(ui->checkBox_change_background, &QCheckBox::toggled, this, [this]() {
        ui->pushButton_background_color->setEnabled(ui->checkBox_change_background->isChecked());
        ui->checkBox_background_grad->setEnabled(ui->checkBox_change_background->isChecked());
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

    updateEnableTargetList();
    createTargetList();

    ui->lineEdit_output2d_output_folder->setDragDropFoloder();

    applyDefault();

    adjustSize();

    updateDlg();
}

VoxOutputDlg::~VoxOutputDlg()
{
    delete m_file_explorer_dlg;
    delete ui;
}

void VoxOutputDlg::optSurfaceXYUpdate()
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

void VoxOutputDlg::optSurfaceYZUpdate()
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

void VoxOutputDlg::optSurfaceXZUpdate()
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

void VoxOutputDlg::optSurfaceXYSetMinMax()
{
    auto voxel = m_gl_widget->firstVoxel();
    if (voxel) {
        ui->sp_min_xy->setValue(0);
        if (ui->radioButton_opt_real_scale->isChecked()) {
            ui->sp_max_xy->setValue(((float)voxel->originalZ() - 1.0f) * voxel->originalDZ() * 1.0e3f);
        }
        else {
            ui->sp_max_xy->setValue(voxel->originalZ() - 1);
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているVoxのMinMaxを設定します。\nVoxが読込まれてないため設定できません。");
    }
}

void VoxOutputDlg::optSurfaceYZSetMinMax()
{
    auto voxel = m_gl_widget->firstVoxel();
    if (voxel) {
        ui->sp_min_yz->setValue(0);
        if (ui->radioButton_opt_real_scale->isChecked()) {
            ui->sp_max_yz->setValue(((float)voxel->originalX() - 1.0f) * voxel->originalDX() * 1.0e3f);
        }
        else {
            ui->sp_max_yz->setValue(voxel->originalX() - 1);
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているVoxのMinMaxを設定します。\nVoxが読込まれてないため設定できません。");
    }
}
void VoxOutputDlg::optSurfaceZXSetMinMax()
{
    auto voxel = m_gl_widget->firstVoxel();
    if (voxel) {
        ui->sp_min_xz->setValue(0);
        if (ui->radioButton_opt_real_scale->isChecked()) {
            ui->sp_max_xz->setValue(((float)voxel->originalY() - 1.0f) * voxel->originalDY() * 1.0e3f);
        }
        else {
            ui->sp_max_xz->setValue(voxel->originalY() - 1);
        }
    }
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているVoxのMinMaxを設定します。\nVoxが読込まれてないため設定できません。");
    }
}

void VoxOutputDlg::optSurfaceXYClear()
{
    ui->le_xy->setText("");
}

void VoxOutputDlg::optSurfaceYZClear()
{
    ui->le_yz->setText("");
}

void VoxOutputDlg::optSurfaceZXClear()
{
    ui->le_xz->setText("");
}

void VoxOutputDlg::optSurfaceAll()
{
    auto voxel = m_gl_widget->firstVoxel();
    if (voxel) {
        ui->sp_min_xz->setValue(0);

        ui->sp_min_yz->setValue(0);

        ui->sp_min_xy->setValue(0);

        if (ui->radioButton_opt_real_scale->isChecked()) {
            ui->sp_max_xy->setValue(((float)voxel->originalZ() - 1.0f) * voxel->originalDZ() * 1.0e3f);
            ui->sp_max_yz->setValue(((float)voxel->originalX() - 1.0f) * voxel->originalDX() * 1.0e3f);
            ui->sp_max_xz->setValue(((float)voxel->originalY() - 1.0f) * voxel->originalDY() * 1.0e3f);

            ui->sp_step_xy->setValue((float)voxel->originalDZ() * 1.0e3f);
            ui->sp_step_yz->setValue((float)voxel->originalDX() * 1.0e3f);
            ui->sp_step_xz->setValue((float)voxel->originalDY() * 1.0e3f);
        }
        else {
            ui->sp_max_xz->setValue(voxel->originalY() - 1);
            ui->sp_max_yz->setValue(voxel->originalX() - 1);
            ui->sp_max_xy->setValue(voxel->originalZ() - 1);

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
    else {
        QMessageBox::warning(this, windowTitle(),
                             "現在読込まれているVoxのMinMaxを設定します。\nVoxが読込まれてないため設定できません。");
    }
}

QString VoxOutputDlg::summarizeValues(const std::set<float>& values, int /* decimals*/)
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

std::set<float> VoxOutputDlg::strToSurfaceValue(QString str, int decimals)
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

QString VoxOutputDlg::correctInputOptSurfaceValue(QString str, int decimals)
{
    return summarizeValues(strToSurfaceValue(str, decimals), decimals);
}

int VoxOutputDlg::valueDecimals(float value)
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

void VoxOutputDlg::updateEnableTargetList()
{
    bool enable_list = false;

    if (ui->radioButton_target_current->isChecked()) {
    }
    else {
        enable_list = true;
    }

    ui->tableWidget_target_list->setEnabled(enable_list);
    ui->pushButton_target_list_all_on->setEnabled(enable_list);
    ui->pushButton_target_list_all_off->setEnabled(enable_list);
    ui->pushButton_target_list_delete->setEnabled(enable_list);
    ui->pushButton_target_list_all_delete->setEnabled(enable_list);
}

void VoxOutputDlg::hideFileExplorerDlg()
{
    if (m_file_explorer_dlg && m_file_explorer_dlg->isVisible()) {
        m_file_explorer_dlg->hide();
    }
}

void VoxOutputDlg::createTargetList()
{
    QStringList labels;
    labels << ""
           << "File Name"
           << "Type"
           << "Folder"
           << "Related File";

    ui->tableWidget_target_list->setColumnCount(labels.size());
    // ui->tableWidget_target_list->setStyleSheet("QTableWidget::item { padding: 0px; }");
    ui->tableWidget_target_list->setHorizontalHeaderLabels(labels);
    // ui->tableWidget_create_auto_dimension->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QVector<int> columnMargins = {10, 150, 15, 150, 200};    // 各列に対する余白の設定

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

    // シグナルとスロットを接続
    QObject::connect(ui->tableWidget_target_list, &QTableWidget::customContextMenuRequested, [this](const QPoint& pos) {
        /// メニュー作成
        QMenu    contextMenu;
        QAction* action_show = contextMenu.addAction("On");
        QAction* action_hide = contextMenu.addAction("Off");
        contextMenu.addSeparator();
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
        else if (selectedAction == action_del) {
            targetListDelete(false);
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
                /*
                QDirIterator it(filePath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    QString subFilePath = it.next();

                    QString ext = QFileInfo(subFilePath).suffix().toLower();
                    if (ext == "vox") {
                        file_paths << subFilePath;
                    }
                    else if (ext == "fdtd") {
                        file_paths << subFilePath;
                    }
                }
                */
                const auto& files = searchFilesWithProgress(filePath);
                if (!files.isEmpty()) {
                    file_paths << files;
                }
            }
            else {
                QString ext = QFileInfo(filePath).suffix().toLower();
                if (ext == "vox") {
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

void VoxOutputDlg::addTargetList(const QStringList& paths)
{
    QStringList vox_files;
    QStringList fdtd_files;

    for (const auto& path : paths) {
        QString ext = QFileInfo(path).suffix().toLower();
        if (ext == "vox") {
            vox_files << path;
        }
        else if (ext == "fdtd") {
            fdtd_files << path;
        }
    }

    ui->tableWidget_target_list->setUpdatesEnabled(false);
    ui->tableWidget_target_list->setSortingEnabled(false);

    int row = ui->tableWidget_target_list->rowCount();
    ui->tableWidget_target_list->setRowCount(row + vox_files.size() + fdtd_files.size());

    for (auto& vox : vox_files) {
        if (addTargetList(vox, row)) {
            ++row;
        }
    }
    for (auto& fdtd : fdtd_files) {
        if (addTargetList(fdtd, row)) {
            ++row;
        }
    }
    ui->tableWidget_target_list->setRowCount(row);
    ui->tableWidget_target_list->setSortingEnabled(true);
    ui->tableWidget_target_list->setUpdatesEnabled(true);

    /// Vox関連付け
    setTargetListRelatedFile();
}

bool VoxOutputDlg::addTargetList(const QString& path, int row)
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

    if (ext == "vox") {
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

    /// Related File
    QTableWidgetItem* related_file_item = new QTableWidgetItem("");
    related_file_item->setFlags(related_file_item->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, related_file_item);

    return true;
}

void VoxOutputDlg::setTargetListRelatedFile()
{
    /// FdtdFiles
    std::map<QString, std::vector<ResultCtrl::FdtdNameData>> key_fdtd_name_data;

    QStringList all_fdtd;

    int row_count = ui->tableWidget_target_list->rowCount();
    for (int row = 0; row < row_count; ++row) {
        const QString& ext = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_TYPE)->text();
        const QString& path =
            ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_FILE_FULLPATH)->data(Qt::UserRole).toString();

        if (ext == "fdtd") {
            ResultCtrl::FdtdNameData data;
            data.m_path   = path;
            data.m_folder = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_FOLDER)->text();

            QString key = QFileInfo(path).completeBaseName().toLower();
            key_fdtd_name_data[key].emplace_back(data);

            all_fdtd << path;
        }
    }

    for (int row = 0; row < row_count; ++row) {
        const QString& ext = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_TYPE)->text();
        const QString& path =
            ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_FILE_FULLPATH)->data(Qt::UserRole).toString();

        if (ext == "vox") {
            auto cell_widget = ui->tableWidget_target_list->cellWidget(row, VOX_OUTPUT_TARGET_LIST_RELATED_FILE);
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

                auto table_item = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_RELATED_FILE);
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

                ui->tableWidget_target_list->setCellWidget(row, VOX_OUTPUT_TARGET_LIST_RELATED_FILE, combo_box);
            }
        }
    }
}

void VoxOutputDlg::targetListCheck(bool check, bool all)
{
    /// listにためる必要ないが他と合わせておく

    std::vector<QCheckBox*>        buttons;
    std::vector<QTableWidgetItem*> button_items;
    QItemSelectionModel*           selection_model = ui->tableWidget_target_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_target_list->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            auto cell_widget = ui->tableWidget_target_list->cellWidget(row, VOX_OUTPUT_TARGET_LIST_TARGET);
            if (cell_widget) {
                auto check_box = cell_widget->findChild<QCheckBox*>();
                if (check_box->isVisible()) {
                    buttons.emplace_back(check_box);
                    button_items.emplace_back(ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_TARGET));
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

void VoxOutputDlg::targetListDelete(bool all)
{
    if (all) {
        m_target_all_files.clear();
    }

    QItemSelectionModel* selection_model = ui->tableWidget_target_list->selectionModel();
    for (int row = ui->tableWidget_target_list->rowCount() - 1; row >= 0; row--) {
        if (all || selection_model->isRowSelected(row)) {
            if (!all) {
                const QString& path = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_FILE_FULLPATH)
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

void VoxOutputDlg::targetListRelatedFileVoxFdtd()
{
    std::vector<QComboBox*>        buttons;
    std::vector<QTableWidgetItem*> button_items;
    QItemSelectionModel*           selection_model = ui->tableWidget_target_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_target_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            const QString& ext = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_TYPE)->text();

            if (ext == "vox") {
                buttons.emplace_back(
                    (QComboBox*)ui->tableWidget_target_list->cellWidget(row, VOX_OUTPUT_TARGET_LIST_RELATED_FILE));
                button_items.emplace_back(ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_RELATED_FILE));
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

void VoxOutputDlg::updateOptSurfaceScale()
{
    if (ui->radioButton_opt_real_scale->isChecked()) {
        int x_decimals = 0;
        int y_decimals = 0;
        int z_decimals = 0;

        float x_step = 1.0f;
        float y_step = 1.0f;
        float z_step = 1.0f;

        auto voxel = m_gl_widget->firstVoxel();
        if (voxel) {
            x_decimals = valueDecimals(voxel->originalDX() * 1e3f);
            y_decimals = valueDecimals(voxel->originalDY() * 1e3f);
            z_decimals = valueDecimals(voxel->originalDZ() * 1e3f);
            x_step     = voxel->originalDX() * 1e3f;
            y_step     = voxel->originalDY() * 1e3f;
            z_step     = voxel->originalDZ() * 1e3f;
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

void VoxOutputDlg::changeOptSurfaceScale()
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

    auto voxel = m_gl_widget->firstVoxel();
    if (voxel) {
        if (ui->radioButton_opt_real_scale->isChecked()) {
            x_step = voxel->originalDX() * 1e3f;
            y_step = voxel->originalDY() * 1e3f;
            z_step = voxel->originalDZ() * 1e3f;
        }
        else {
            x_step = 1.0f / (voxel->originalDX() * 1e3f);
            y_step = 1.0f / (voxel->originalDY() * 1e3f);
            z_step = 1.0f / (voxel->originalDZ() * 1e3f);
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

void VoxOutputDlg::addValuesToLineEdit(QCheckBox* checkBox, QDoubleSpinBox* minSpinBox, QDoubleSpinBox* maxSpinBox,
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

std::vector<VoxOutputDlg::PlaneInfo> VoxOutputDlg::optTargetSurface()
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

void VoxOutputDlg::setDefault()
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
    setting_gui_ctrl->saveVoxOutputImageFormat(picture_ext);

    int size_type =
        ui->radioButton_picture_size_data->isChecked() ? 0 : (ui->radioButton_picture_size_view->isChecked() ? 1 : 2);
    setting_gui_ctrl->saveVoxOutputImageSizeType(size_type);

    setting_gui_ctrl->saveVoxOutputImageSizeRatio(ui->doubleSpinBox_picture_size_data->value());
    setting_gui_ctrl->saveVoxOutputImageSizeW(ui->spinBox_picture_size_specify_w->value());
    setting_gui_ctrl->saveVoxOutputImageSizeH(ui->spinBox_picture_size_specify_h->value());

    setting_gui_ctrl->saveVoxOutputImageDispVox(ui->checkBox_outline_vox_show_hide->isChecked());
    setting_gui_ctrl->saveVoxOutputImageFitDispVox(ui->checkBox_outline_vox_show_fit->isChecked());

    setting_gui_ctrl->saveVoxOutputImageBackgroundChange(ui->checkBox_change_background->isChecked());
    setting_gui_ctrl->saveVoxOutputImageBackgroundGrad(ui->checkBox_background_grad->isChecked());
    setting_gui_ctrl->saveVoxOutputImageBackgroundColor(m_background_color);
}

void VoxOutputDlg::applyDefault(bool show_msg)
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

    int picture_ext = setting_gui_ctrl->readVoxOutputImageFormat();
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

    int size_type = setting_gui_ctrl->readVoxOutputImageSizeType();
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

    ui->doubleSpinBox_picture_size_data->setValue(setting_gui_ctrl->readVoxOutputImageSizeRatio());
    ui->spinBox_picture_size_specify_w->setValue(setting_gui_ctrl->readVoxOutputImageSizeW());
    ui->spinBox_picture_size_specify_h->setValue(setting_gui_ctrl->readVoxOutputImageSizeH());

    ui->checkBox_outline_vox_show_hide->setChecked(setting_gui_ctrl->readVoxOutputImageDispVox());
    ui->checkBox_outline_vox_show_fit->setChecked(setting_gui_ctrl->readVoxOutputImageFitDispVox());

    ui->checkBox_change_background->setChecked(setting_gui_ctrl->readVoxOutputImageBackgroundChange());
    ui->checkBox_background_grad->setChecked(setting_gui_ctrl->readVoxOutputImageBackgroundGrad());
    m_background_color = setting_gui_ctrl->readVoxOutputImageBackgroundColor();
    ui->pushButton_background_color->setStyleSheet(Vox3DForm::colorStyle(m_background_color));
}

void VoxOutputDlg::initDefault()
{
    SettingGuiCtrl* setting_gui_ctrl = m_3DForm->settingGuiCtrl();
    if (!setting_gui_ctrl) {
        return;
    }

    if (QMessageBox::question(this, windowTitle(), "Image Settingのデフォルト設定を初期化します。")
        != QMessageBox::Yes) {
        return;
    }

    setting_gui_ctrl->initVoxOutputImage();
}

void VoxOutputDlg::setProgMessage(int value)
{
    if (m_pd) {
        m_pd->setValue(value);
    }
}

void VoxOutputDlg::setProgText(const QString& text)
{
    if (m_pd) {
        m_pd->setLabelText(text);
    }
}

void VoxOutputDlg::setProgEnd()
{
    if (m_pd) {
        delete m_pd;
        m_pd = nullptr;
    }
}

void VoxOutputDlg::optSurfaceXYAdd()
{
    addValuesToLineEdit(ui->cb_xy, ui->sp_min_xy, ui->sp_max_xy, ui->sp_step_xy, ui->le_xy);
}

void VoxOutputDlg::optSurfaceYZAdd()
{
    addValuesToLineEdit(ui->cb_yz, ui->sp_min_yz, ui->sp_max_yz, ui->sp_step_yz, ui->le_yz);
}

void VoxOutputDlg::optSurfaceZXAdd()
{
    addValuesToLineEdit(ui->cb_xz, ui->sp_min_xz, ui->sp_max_xz, ui->sp_step_xz, ui->le_xz);
}

void VoxOutputDlg::updateDlg()
{
    int width  = m_gl_widget->sceneView()->viewPortWidth();
    int height = m_gl_widget->sceneView()->viewPortHeight();

    ui->lineEdit_picture_size_view->setText(QString("%1 * %2").arg(width).arg(height));

    updateOptSurfaceScale();
}

void VoxOutputDlg::outputPicture()
{
    /// とりあえず非表示
    hideFileExplorerDlg();

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

    VoxOutputThread::Input input;

    std::vector<VoxOutputThread::InputGroup>& input_groups = input.m_input_groups;
    if (ui->radioButton_target_folder->isChecked()) {
        std::map<QString, VoxOutputThread::InputGroup> map_group;

        for (int row = 0; row < ui->tableWidget_target_list->rowCount(); ++row) {
            const QString& ext  = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_TYPE)->text();
            const QString& path = ui->tableWidget_target_list->item(row, VOX_OUTPUT_TARGET_LIST_FILE_FULLPATH)
                                      ->data(Qt::UserRole)
                                      .toString();

            if (ext == "vox") {
                auto cell_widget = ui->tableWidget_target_list->cellWidget(row, VOX_OUTPUT_TARGET_LIST_TARGET);
                if (cell_widget) {
                    QCheckBox* check_box = (QCheckBox*)cell_widget->findChild<QCheckBox*>();
                    if (!check_box->isChecked()) {
                        continue;
                    }
                }

                QString vox_file = path;

                auto& group      = map_group[vox_file.toLower()];
                group.m_vox_file = path;

                QString fdtd_file;

                auto cell_widget_2 = ui->tableWidget_target_list->cellWidget(row, VOX_OUTPUT_TARGET_LIST_RELATED_FILE);
                if (cell_widget_2) {
                    fdtd_file = ((QComboBox*)cell_widget_2)->currentText();
                    if (fdtd_file == "(None)") {
                        fdtd_file = "";
                    }
                }

                group.m_fdtd_file = fdtd_file;
            }
        }

        for (auto& [key, group] : map_group) {
            if (!group.m_vox_file.isEmpty()) {
                input_groups.emplace_back(group);
            }
        }

        /// データなし
        if (input_groups.size() == 0) {
            QMessageBox::warning(this, windowTitle(), "There is no target file.");
            return;
        }
    }
    else {
        if (!m_gl_widget->firstVoxel(ui->checkBox_outline_vox_show_hide->isChecked())) {
            QMessageBox::warning(this, windowTitle(), "There is no target data.");
            return;
        }
    }

    QString picture_ext = ui->radioButton_pictureformat_png->isChecked()
                            ? ".png"
                            : (ui->radioButton_pictureformat_jpg->isChecked() ? ".jpg" : ".bmp");

    float data_size_ratio = 0;
    int   pic_width       = 0;
    int   pic_height      = 0;

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

    input.m_target_surface = optTargetSurface();
    if (input.m_target_surface.size() == 0) {
        QMessageBox::warning(this, windowTitle(), "There is no target surface.");
        return;
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

    input.m_ext                     = picture_ext;
    input.m_data_size_ratio         = data_size_ratio;
    input.m_pic_width               = pic_width;
    input.m_pic_height              = pic_height;
    input.m_current_show            = outline_current_show;
    input.m_vox_current_shading     = current_shading;
    input.m_vox_current_wireframe   = current_wireframe;
    input.m_background_color_change = ui->checkBox_change_background->isChecked();
    input.m_background_color        = m_background_color;
    input.m_background_grad         = ui->checkBox_background_grad->isChecked();
    input.m_show_fit                = ui->checkBox_outline_vox_show_fit->isChecked();
    input.m_output_folder           = output_folder;

    /// GUI関連のバックアップ
    std::vector<char>               back_compress_data      = m_3DForm->resultCtl()->voxelFileCompressData();
    int                             back_compress_data_size = m_3DForm->resultCtl()->compressDataOriginalSize();
    std::map<int, WeakRefPtr<Node>> material_id_nodes       = m_gl_widget->materialIdNodes();

    VoxOutputThread output_thread(this, m_3DForm, m_gl_widget, input);
    connect(&output_thread, &VoxOutputThread::setProgMessage, this, &VoxOutputDlg::setProgMessage,
            Qt::QueuedConnection);
    connect(&output_thread, &VoxOutputThread::setProgText, this, &VoxOutputDlg::setProgText, Qt::QueuedConnection);
    connect(&output_thread, &VoxOutputThread::setProgEnd, this, &VoxOutputDlg::setProgEnd, Qt::QueuedConnection);

    connect(&output_thread, &VoxOutputThread::finished, this, [&thread_end] { thread_end = true; });
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

void VoxOutputThread::collectTarget(Node* node)
{
    TargetNode target;
    target.m_node        = node;
    target.m_show        = node->isShow();
    target.m_suppress    = node->isSuppress();
    target.m_shading     = false;
    target.m_wireframe   = false;
    target.m_transparent = 1.0f;
    target.m_proj_opt    = nullptr;

    auto object = node->object();
    if (object) {
        switch (object->type()) {
            case ObjectType::Voxel:
                target.m_shading     = node->isDrawShading();
                target.m_wireframe   = node->isDrawWireframe();
                target.m_transparent = node->transparent();
                target.m_proj_opt    = node->projectionNode();
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

void VoxOutputThread::run()
{
    outputPicture();
}

void VoxOutputThread::outputPicture()
{
    m_gl_widget->suppressRender(true);

    if (!m_input.m_input_groups.empty()) {
        int output_group_count    = 0;
        int output_group_all_size = m_input.m_input_groups.size();

        for (int ic = 0; ic < m_input.m_input_groups.size(); ++ic) {
            if (m_cancel) {
                break;
            }
            m_gl_widget->sceneView()->sceneGraph()->resetTemporaryRoot();
            m_gl_widget->sceneView()->sceneGraph()->setTemporaryRoot();    /// 現在のデータを隠して空にする

            /// ファイルロード
            auto& group = m_input.m_input_groups[ic];

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

            for (auto& vox : m_voxel_list) {
                vox.m_node->setDrawShading(m_input.m_vox_current_shading);
                vox.m_node->setDrawWireframe(m_input.m_vox_current_wireframe);
            }

            ++output_group_count;

            if (m_voxel_list.size() == 0) {
                m_voxel_list.clear();
                m_opt2d_list.clear();
                m_opt3d_list.clear();
                m_other_list.clear();
                continue;
            }

            if (m_cancel) {
                break;
            }

            outputPictureOneGroup(QString("Output Image %1 / %2").arg(output_group_count).arg(output_group_all_size));

            m_voxel_list.clear();
            m_opt2d_list.clear();
            m_opt3d_list.clear();
            m_other_list.clear();

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

        if (m_voxel_list.size() == 0) {
        }
        else {
            outputPictureOneGroup("Output Image");
        }

        m_voxel_list.clear();
        m_opt2d_list.clear();
        m_opt3d_list.clear();
        m_other_list.clear();
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

    m_gl_widget->suppressRender(false);
}

void VoxOutputThread::outputPictureOneGroup(const QString& label)
{
    /// GUI処理、描画処理はメインスレッドで行う必要がある（同期とって待機する）
    ///  ※ invokeMethodを使う

    const auto& output_folder = m_input.m_output_folder;

    auto picture_ext     = m_input.m_ext;
    auto data_size_ratio = m_input.m_data_size_ratio;
    auto pic_width       = m_input.m_pic_width;
    auto pic_height      = m_input.m_pic_height;

    auto current_show = m_input.m_current_show;
    auto show_fit     = m_input.m_show_fit;

    const auto& opt_planes = m_input.m_target_surface;

    for (auto& target : m_voxel_list) {
        if (!current_show) {
            target.m_node->setDrawShading(true);
            target.m_node->setDrawWireframe(true);
            target.m_node->setTransparent(1.0f);
            target.m_node->setProjectionNode(nullptr);
            target.m_node->show();
        }
        // voxel->setTransparent(1.0f);
        // voxel->setProjectionOpt(nullptr);
    }
    for (auto& target : m_opt2d_list) {
        target.m_node->hide();
    }
    for (auto& target : m_opt3d_list) {
        target.m_node->suppress();    /// 抑制にする
    }
    for (auto& target : m_other_list) {
        target.m_node->hide();
    }

    auto save_scene = m_gl_widget->sceneView()->saveCameraState();
    m_gl_widget->sceneView()->setProjectionType(SceneView::ProjectionType::Ortho);

    const auto save_background_color    = m_gl_widget->backgroundColor();
    const auto save_background_gradient = m_gl_widget->backGroundColorGradient();

    if (m_input.m_background_color_change) {
        m_gl_widget->setBackgroundColor(m_input.m_background_color, false);
        m_gl_widget->setBackGroundColorGradient(m_input.m_background_grad, false);
    }

    const auto wireframe_color       = m_gl_widget->voxelWireframeColor();
    float      wireframe_width       = m_gl_widget->voxelWireframeWidth();
    bool       wireframe_color_shape = m_gl_widget->isVoxelWireframeColorShape();

    if (!current_show) {
        m_gl_widget->setVoxelWireframeColor(Point4f(0, 0, 0, 0), false);
        m_gl_widget->setVoxelWireframeWidth(1.0f / m_gl_widget->devicePixelRatio(), false);
        m_gl_widget->setVoxelWireframeColorShape(false, false);
    }
    else {
        m_gl_widget->setVoxelWireframeWidth(wireframe_width / m_gl_widget->devicePixelRatio(), false);
    }

    int prog_min   = 0;
    int prog_max   = 100;
    int prog_step  = 1;
    int prog_range = (prog_max - prog_min) / prog_step + 1;

    int data_all_size = opt_planes.size();
    int prog_count    = data_all_size / prog_range + 1;
    int data_count    = 0;

    emit setProgText(label);
    emit setProgMessage(prog_min);

    auto& set_basename = m_set_basename;

    if (!opt_planes.empty() && !m_voxel_list.empty()) {
        for (int dummy = 0; dummy < 1; ++dummy) {    /// Dummy　ループはしない(breakとcontinue修正面倒でこれ）
            if (m_cancel) {
                break;
            }

            /// Vox
            BoundingBox3f bbox;
            BoundingBox3f bbox_disp;
            for (auto& target : m_voxel_list) {
                auto target_box = target.m_node->calculateBoundingBox(Matrix4x4f(), false);
                bbox.expandBy(target_box);
                if (show_fit) {
                    if (!target.m_node->isVisible()) {
                        continue;
                    }
                    bbox_disp.expandBy(target_box);
                }
            }

            float x_vox_disp_ratio = show_fit ? bbox_disp.getXLength() / bbox.getXLength() : 1.0f;
            float y_vox_disp_ratio = show_fit ? bbox_disp.getYLength() / bbox.getYLength() : 1.0f;
            float z_vox_disp_ratio = show_fit ? bbox_disp.getZLength() / bbox.getZLength() : 1.0f;

            if (show_fit) {
                bbox = bbox_disp;
            }

            Voxel* voxel = m_voxel_list[0].m_node->object<Voxel>();
            bbox.shrinkBy(-voxel->dX() * 0.01, -voxel->dY() * 0.01, -voxel->dZ() * 0.01);

            const std::wstring& file_path = m_voxel_list[0].m_node->userAttributeString(L"File Path");
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
                dirPath = fileInfo.absoluteDir().absolutePath() + QDir::separator() + QString("pics_vox");
            }
            else {
                dirPath = output_folder + QDir::separator() + QString("pics_vox");
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
                        nx = (int)((float)voxel->originalX() * x_vox_disp_ratio);    /// 画像サイズ調整のため
                        ny = (int)((float)voxel->originalZ() * z_vox_disp_ratio);
                        nz = (int)((float)voxel->originalY() * 1.0f);    /// nzはそのまま
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
                        nx = (int)((float)voxel->originalY() * y_vox_disp_ratio);
                        ny = (int)((float)voxel->originalZ() * z_vox_disp_ratio);
                        nz = (int)((float)voxel->originalX() * 1.0f);
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
                        nx = (int)((float)voxel->originalX() * x_vox_disp_ratio);
                        ny = (int)((float)voxel->originalY() * y_vox_disp_ratio);
                        nz = (int)((float)voxel->originalZ() * 1.0f);
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

                m_3DForm->resultCtl()->viewOnPlaneNoChangeGUI(plane);

                /// クリッピングした結果データなしか
                bool has_disp_data = false;
                for (auto& target : m_voxel_list) {
                    if (!target.m_node->isVisible()) {
                        continue;
                    }

                    RenderEditableMesh* mesh = (RenderEditableMesh*)target.m_node->renderable();

                    const auto& indices =
                        mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

                    if (indices.size() > 0) {
                        has_disp_data = true;
                        break;
                    }
                }
                if (!has_disp_data) {
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

                m_gl_widget->sceneView()->fitDisplayObjectBox(bbox, 1.0f);

                QImage image;

                QMetaObject::invokeMethod(
                    this,
                    [this, &width, &height, &image]() {
                        m_gl_widget->updateRenderData();
                        m_gl_widget->outputResultImage(width, height, false, false, image);
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
        }
    }

    m_gl_widget->sceneView()->loadCameraState(save_scene.ptr());

    m_gl_widget->setBackgroundColor(save_background_color, false);
    m_gl_widget->setBackGroundColorGradient(save_background_gradient, false);

    m_gl_widget->setVoxelWireframeColor(wireframe_color, false);
    m_gl_widget->setVoxelWireframeWidth(wireframe_width, false);
    m_gl_widget->setVoxelWireframeColorShape(wireframe_color_shape, false);

    for (auto& target : m_voxel_list) {
        if (!current_show) {
            target.m_node->setDrawShading(target.m_shading);
            target.m_node->setDrawWireframe(target.m_wireframe);
            if (target.m_show)
                target.m_node->show();
            else
                target.m_node->hide();
        }
        target.m_node->setTransparent(target.m_transparent);
        target.m_node->setProjectionNode(target.m_proj_opt);
    }
    for (auto& target : m_opt2d_list) {
        if (target.m_show)
            target.m_node->show();
        else
            target.m_node->hide();
    }
    for (auto& target : m_opt3d_list) {
        if (target.m_suppress)
            target.m_node->suppress();
        else
            target.m_node->unsuppress();
    }
    for (auto& target : m_other_list) {
        if (target.m_show)
            target.m_node->show();
        else
            target.m_node->hide();
    }
}
