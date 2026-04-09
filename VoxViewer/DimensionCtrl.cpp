#include "DimensionCtrl.h"
#include <QWidget>
#include "ClippingCtrl.h"
#include "MyOpenGLWidget.h"
#include "ui_Vox3DForm.h"

#include <QColorDialog>
#include <QMenu>

#include <QPainter>
#include <QStyleOptionButton>

#include "Scene/Dimension.h"
#include "Scene/MultiDimension.h"
#include "Scene/SceneView.h"
#include "Utils/Clipping.h"
#include "Utils/Picking.h"

#define DIMENSION_LIST_VIEW 0
#define DIMENSION_LIST_NAME 1
#define DIMENSION_LIST_DIST 2
#define DIMENSION_LIST_COL 3
#define DIMENSION_LIST_SIZE 4
#define DIMENSION_LIST_WIDTH 5
#define DIMENSION_LIST_EDGE 6
#define DIMENSION_LIST_START_X 7
#define DIMENSION_LIST_START_Y 8
#define DIMENSION_LIST_START_Z 9
#define DIMENSION_LIST_END_X 10
#define DIMENSION_LIST_END_Y 11
#define DIMENSION_LIST_END_Z 12

#define DIMENSION_NODE_ITEM DIMENSION_LIST_START_X

#define AUTO_DIMENSION_LIST_VIEW 0
#define AUTO_DIMENSION_LIST_NAME 1
#define AUTO_DIMENSION_LIST_VAL 2
#define AUTO_DIMENSION_LIST_MAT 3
#define AUTO_DIMENSION_LIST_COL 4
#define AUTO_DIMENSION_LIST_SIZE 5
#define AUTO_DIMENSION_LIST_WIDTH 6
#define AUTO_DIMENSION_LIST_EDGE 7
#define AUTO_DIMENSION_LIST_ALIGN 8
#define AUTO_DIMENSION_LIST_SECTION 9
#define AUTO_DIMENSION_LIST_START_X 10
#define AUTO_DIMENSION_LIST_START_Y 11
#define AUTO_DIMENSION_LIST_START_Z 12
#define AUTO_DIMENSION_LIST_END_X 13
#define AUTO_DIMENSION_LIST_END_Y 14
#define AUTO_DIMENSION_LIST_END_Z 15

#define AUTO_DIMENSION_NODE_ITEM AUTO_DIMENSION_LIST_START_X

class SpinBoxDialog : public QDialog {
public:
    explicit SpinBoxDialog(const QString& title, QWidget* parent = nullptr) : QDialog(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // QSpinBox を作成
        m_spin = new CustomSpin(this);
        m_spin->setRange(1, 999);    // 範囲を設定
        mainLayout->addWidget(m_spin);

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
    CustomSpin* m_spin;
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

class LineEditDialog : public QDialog {
public:
    explicit LineEditDialog(const QString& title, QWidget* parent = nullptr) : QDialog(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // QLineEdit を作成
        m_edit = new QLineEdit(this);
        mainLayout->addWidget(m_edit);

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
    QLineEdit* m_edit;
};

DimensionCtrl::DimensionCtrl(Vox3DForm* parent, Ui::Vox3DForm* ui_) : QObject(parent), ui(ui_), m_3DForm(parent)
{
    /// Drag Filter
    ui->checkBox_create_dimension_text_drag->setChecked(true);
    ui->checkBox_create_auto_dimension_text_drag->setChecked(false);
    ui->obj3dViewer->addDragFilter(ObjectType::Dimension);
    connect(ui->checkBox_create_dimension_text_drag, &QCheckBox::toggled, this, [this](bool) {
        ui->obj3dViewer->clearDragFilter();
        if (ui->checkBox_create_dimension_text_drag->isChecked()) {
            ui->obj3dViewer->addDragFilter(ObjectType::Dimension);
        }
        if (ui->checkBox_create_auto_dimension_text_drag->isChecked()) {
            ui->obj3dViewer->addDragFilter(ObjectType::MultiDimension);
        }
    });
    connect(ui->checkBox_create_auto_dimension_text_drag, &QCheckBox::toggled, this, [this](bool) {
        ui->obj3dViewer->clearDragFilter();
        if (ui->checkBox_create_dimension_text_drag->isChecked()) {
            ui->obj3dViewer->addDragFilter(ObjectType::Dimension);
        }
        if (ui->checkBox_create_auto_dimension_text_drag->isChecked()) {
            ui->obj3dViewer->addDragFilter(ObjectType::MultiDimension);
        }
    });

    ui->pushButton_create_dimension_color->setStyleSheet(Vox3DForm::colorStyle(m_dimension_color));
    ui->pushButton_create_auto_dimension_color->setStyleSheet(Vox3DForm::colorStyle(m_auto_dimension_color));

    connect(ui->pushButton_create_dimension, &QPushButton::clicked, this, [this](bool) {
        if (ui->pushButton_create_dimension->isChecked()) {
            ui->pushButton_create_auto_dimension->blockSignals(true);
            ui->pushButton_create_auto_dimension->setChecked(false);
            ui->pushButton_create_auto_dimension->blockSignals(false);
        }
        ui->obj3dViewer->setPickMode(
            ui->pushButton_create_dimension->isChecked() || ui->pushButton_create_auto_dimension->isChecked(),
            dimensionSnap());
        ui->obj3dViewer->pickClear();

        if (ui->pushButton_create_dimension->isChecked()) {
            ui->obj3dViewer->setPickColor(m_dimension_color);
        }
        else if (ui->pushButton_create_auto_dimension->isChecked()) {
            ui->obj3dViewer->setPickColor(m_auto_dimension_color);
        }
    });

    connect(ui->pushButton_create_auto_dimension, &QPushButton::clicked, this, [this](bool) {
        if (ui->pushButton_create_auto_dimension->isChecked()) {
            ui->pushButton_create_dimension->blockSignals(true);
            ui->pushButton_create_dimension->setChecked(false);
            ui->pushButton_create_dimension->blockSignals(false);
        }
        ui->obj3dViewer->setPickMode(
            ui->pushButton_create_auto_dimension->isChecked() || ui->pushButton_create_auto_dimension->isChecked(),
            dimensionSnap());
        ui->obj3dViewer->pickClear();

        if (ui->pushButton_create_dimension->isChecked()) {
            ui->obj3dViewer->setPickColor(m_dimension_color);
        }
        else if (ui->pushButton_create_auto_dimension->isChecked()) {
            ui->obj3dViewer->setPickColor(m_auto_dimension_color);
        }
    });

    connect(ui->checkBox_create_dimension_snap, &QCheckBox::toggled, this, [this](bool) {
        ui->obj3dViewer->setPickMode(
            ui->pushButton_create_dimension->isChecked() || ui->pushButton_create_auto_dimension->isChecked(),
            dimensionSnap());
        ui->checkBox_create_dimension_snap_2->blockSignals(true);
        ui->checkBox_create_dimension_snap_2->setChecked(ui->checkBox_create_dimension_snap->isChecked());
        ui->checkBox_create_dimension_snap_2->blockSignals(false);
    });

    connect(ui->checkBox_create_dimension_snap_2, &QCheckBox::toggled, this, [this](bool) {
        ui->checkBox_create_dimension_snap->blockSignals(true);
        ui->checkBox_create_dimension_snap->setChecked(ui->checkBox_create_dimension_snap_2->isChecked());
        ui->checkBox_create_dimension_snap->blockSignals(false);
        ui->obj3dViewer->setPickMode(
            ui->pushButton_create_dimension->isChecked() || ui->pushButton_create_auto_dimension->isChecked(),
            dimensionSnap());
    });

    connect(ui->comboBox_ceate_dimension_snap, &QComboBox::currentTextChanged, this, [this](const QString&) {
        ui->obj3dViewer->setPickMode(
            ui->pushButton_create_dimension->isChecked() || ui->pushButton_create_auto_dimension->isChecked(),
            dimensionSnap());
        ui->comboBox_ceate_dimension_snap_2->blockSignals(true);
        ui->comboBox_ceate_dimension_snap_2->setCurrentIndex(ui->comboBox_ceate_dimension_snap->currentIndex());
        ui->comboBox_ceate_dimension_snap_2->blockSignals(false);
    });

    connect(ui->comboBox_ceate_dimension_snap_2, &QComboBox::currentTextChanged, this, [this](const QString&) {
        ui->comboBox_ceate_dimension_snap->blockSignals(true);
        ui->comboBox_ceate_dimension_snap->setCurrentIndex(ui->comboBox_ceate_dimension_snap_2->currentIndex());
        ui->comboBox_ceate_dimension_snap->blockSignals(false);
        ui->obj3dViewer->setPickMode(
            ui->pushButton_create_dimension->isChecked() || ui->pushButton_create_auto_dimension->isChecked(),
            dimensionSnap());
    });

    connect(ui->pushButton_create_dimension_clear, &QPushButton::clicked, this,
            [this](bool) { ui->obj3dViewer->clearDimensions(); });

    connect(ui->pushButton_create_dimension_clear_selected, &QPushButton::clicked, this,
            [this](bool) { removeDimension(); });

    connect(ui->pushButton_create_dimension_all_on, &QPushButton::clicked, this,
            [this](bool) { dimensionShow(true, true); });

    connect(ui->pushButton_create_dimension_all_off, &QPushButton::clicked, this,
            [this](bool) { dimensionShow(false, true); });

    connect(ui->pushButton_create_auto_dimension_clear, &QPushButton::clicked, this,
            [this](bool) { ui->obj3dViewer->clearAutoDimensions(); });

    connect(ui->pushButton_create_auto_dimension_clear_selected, &QPushButton::clicked, this,
            [this](bool) { removeAutoDimension(); });

    connect(ui->pushButton_create_auto_dimension_all_on, &QPushButton::clicked, this,
            [this](bool) { autoDimensionShow(true, true); });

    connect(ui->pushButton_create_auto_dimension_all_off, &QPushButton::clicked, this,
            [this](bool) { autoDimensionShow(false, true); });

    // ボタンがクリックされたときに色選択ダイアログを表示
    connect(ui->pushButton_create_dimension_color, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_dimension_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            m_dimension_color = selectedColor;
            // ボタンの背景色を選択した色に設定
            ui->pushButton_create_dimension_color->setStyleSheet(Vox3DForm::colorStyle(m_dimension_color));
            if (ui->pushButton_create_dimension->isChecked()) {
                ui->obj3dViewer->setPickColor(m_dimension_color);
            }
        }
    });

    connect(ui->pushButton_create_auto_dimension_color, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_auto_dimension_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            m_auto_dimension_color = selectedColor;
            // ボタンの背景色を選択した色に設定
            ui->pushButton_create_auto_dimension_color->setStyleSheet(Vox3DForm::colorStyle(m_auto_dimension_color));
            if (ui->pushButton_create_auto_dimension->isChecked()) {
                ui->obj3dViewer->setPickColor(m_auto_dimension_color);
            }
        }
    });

    connect(ui->checkBox_create_dimension_disp_name, &QCheckBox::toggled, this, [this](bool) {
        if (m_temporary_dimension != nullptr) {
            // if (m_temporary_dimension.isAlive()) {
            Dimension* dim = (Dimension*)m_temporary_dimension->object();
            dim->setDispName(ui->checkBox_create_dimension_disp_name->isChecked());
            ui->obj3dViewer->update();
        }
    });

    connect(ui->checkBox_create_auto_dimension_disp_name, &QCheckBox::toggled, this, [this](bool) {
        if (m_continuous_auto_dimension != nullptr) {
            // if (m_continuous_auto_dimension.isAlive()) {
            MultiDimension* dim = (MultiDimension*)m_continuous_auto_dimension->object();
            dim->setDispName(ui->checkBox_create_auto_dimension_disp_name->isChecked());
            ui->obj3dViewer->update();
        }
    });

    createDimensionList();

    ui->comboBox_create_auto_dimension_type->setCurrentText("2D Points");
    ui->checkBox_create_auto_dimension_extend->setChecked(true);

    /// 一旦 既存の2Dビューアと同等機能（縦横のみ1点指定可能）で実装。2点とか連続考慮がいろいろ面倒なので
    ui->label_create_auto_dimension_pos1->setEnabled(false);
    ui->label_create_auto_dimension_pos_1_x->setEnabled(false);
    ui->label_create_auto_dimension_pos_1_y->setEnabled(false);
    ui->label_create_auto_dimension_pos_1_z->setEnabled(false);
    ui->doubleSpinBox_create_auto_dimension_pos1_x->setEnabled(false);
    ui->doubleSpinBox_create_auto_dimension_pos1_y->setEnabled(false);
    ui->doubleSpinBox_create_auto_dimension_pos1_z->setEnabled(false);
    ui->pushButton_create_auto_dimension_pos_preview->setEnabled(false);
    ui->pushButton_create_auto_dimension_pos_create->setEnabled(false);
    ui->label_create_auto_dimension_pos_2_x->setVisible(false);
    ui->label_create_auto_dimension_pos_2_y->setVisible(false);
    ui->label_create_auto_dimension_pos_2_z->setVisible(false);
    ui->doubleSpinBox_create_auto_dimension_pos2_x->setVisible(false);
    ui->doubleSpinBox_create_auto_dimension_pos2_y->setVisible(false);
    ui->doubleSpinBox_create_auto_dimension_pos2_z->setVisible(false);

    connect(ui->checkBox_create_auto_dimension_extend, &QCheckBox::toggled, this,
            [this](bool) { createAutoDimensionGUIUpdate(); });

    connect(ui->comboBox_create_auto_dimension_type, &QComboBox::currentTextChanged, this,
            [this](const QString&) { createAutoDimensionGUIUpdate(); });

    connect(ui->pushButton_create_auto_dimension_pos_create, &QPushButton::clicked, this,
            [this]() { createAutoDimensionByPoint(false); });
    connect(ui->pushButton_create_auto_dimension_pos_preview, &QPushButton::toggled, this, [this]() {
        if (ui->pushButton_create_auto_dimension_pos_preview->isChecked()) {
            createAutoDimensionByPoint(true);
        }
        else {
            clearTemporaryPreviewAutoDimension();
        }
    });
    connect(ui->doubleSpinBox_create_auto_dimension_pos1_x, &QDoubleSpinBox::valueChanged, this, [this]() {
        if (ui->pushButton_create_auto_dimension_pos_preview->isChecked()) {
            createAutoDimensionByPoint(true);
        }
    });
    connect(ui->doubleSpinBox_create_auto_dimension_pos1_y, &QDoubleSpinBox::valueChanged, this, [this]() {
        if (ui->pushButton_create_auto_dimension_pos_preview->isChecked()) {
            createAutoDimensionByPoint(true);
        }
    });
    connect(ui->doubleSpinBox_create_auto_dimension_pos1_z, &QDoubleSpinBox::valueChanged, this, [this]() {
        if (ui->pushButton_create_auto_dimension_pos_preview->isChecked()) {
            createAutoDimensionByPoint(true);
        }
    });

    ui->checkBox_create_auto_dimension_disp_value->setChecked(true);
    ui->checkBox_create_auto_dimension_disp_material->setChecked(true);

    ui->checkBox_create_auto_dimension_along_plane->setChecked(true);
    createAutoDimensionGUIUpdate();

    createAutoDimensionList();
    createAutoDimensionText();

    // QFont text_edit_font("Consolas");
    // int   font_size = QApplication::font().pointSize();
    // if (font_size < 10) font_size = 10;
    // text_edit_font.setPointSize(font_size);
    // ui->textEdit_create_auto_dimension->setFont(text_edit_font);

    connect(ui->checkBox_create_dimension_unit_disp, &QCheckBox::toggled, this, [this](bool) {
        for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            Dimension* dimension = (Dimension*)node->object();
            dimension->setDisplayUnit(ui->checkBox_create_dimension_unit_disp->isChecked());
            dimension->updateText();
        }
        ui->obj3dViewer->update();
    });
    connect(ui->checkBox_create_auto_dimension_unit_disp, &QCheckBox::toggled, this, [this](bool) {
        for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            MultiDimension* dimension = (MultiDimension*)node->object();
            dimension->setDisplayUnit(ui->checkBox_create_auto_dimension_unit_disp->isChecked());
            dimension->updateText();
        }
        ui->obj3dViewer->update();
    });
}

Dimension::TextAlign DimensionCtrl::autoDimensionTextAlign(QComboBox* combobox) const
{
    const auto& text =
        combobox ? combobox->currentText() : ui->comboBox_create_auto_dimension_text_align->currentText();
    if (text == "Auto") {
        return Dimension::TextAlign::Auto;
    }
    if (text == "Horizontal" || text == "H") {
        return Dimension::TextAlign::Horizontal;
    }
    if (text == "Vertical" || text == "V") {
        return Dimension::TextAlign::Vertical;
    }
    return Dimension::TextAlign::Auto;
}

void DimensionCtrl::createDimensionList()
{
    QStringList labels;
    labels << "View"
           << "Name"
           << "Distance"
           << "Col"
           << "Size"
           << "Width"
           << "Edge"
           << "X"
           << "Y"
           << "Z"
           << "X"
           << "Y"
           << "Z";
    ui->tableWidget_create_dimension_list->setColumnCount(labels.size());
    ui->tableWidget_create_dimension_list->setHorizontalHeaderLabels(labels);
    // ui->tableWidget_create_dimension_list->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QVector<int> columnMargins = {5, 25, 10, 10, 20, 10, 15, 43, 43, 43, 43, 43, 43};    // 各列に対する余白の設定

    QFontMetrics font_metrics(ui->tableWidget_create_dimension_list->horizontalHeader()->font());
    for (int i = 0; i < labels.size(); ++i) {
        int size = font_metrics.horizontalAdvance(labels[i]);

        ui->tableWidget_create_dimension_list->setColumnWidth(i, size + columnMargins[i]);
    }

    // カスタムコンテキストメニューを有効化
    ui->tableWidget_create_dimension_list->setContextMenuPolicy(Qt::CustomContextMenu);

    // シグナルとスロットを接続
    QObject::connect(ui->tableWidget_create_dimension_list, &QTableWidget::customContextMenuRequested,
                     [this](const QPoint& pos) {
                         // 右クリックメニューを作成
                         QMenu contextMenu;

                         // メニュー項目を追加
                         QAction* action_show = contextMenu.addAction("表示");
                         QAction* action_hide = contextMenu.addAction("非表示");
                         contextMenu.addSeparator();
                         QMenu    sub_dimension_name("寸法名");
                         QAction* action_dim_show   = sub_dimension_name.addAction("表示");
                         QAction* action_dim_hide   = sub_dimension_name.addAction("非表示");
                         QAction* action_dim_change = sub_dimension_name.addAction("変更");
                         contextMenu.addMenu(&sub_dimension_name);
                         contextMenu.addSeparator();
                         QAction* action_color     = contextMenu.addAction("色変更");
                         QAction* action_size      = contextMenu.addAction("サイズ変更");
                         QAction* action_linewidth = contextMenu.addAction("線幅変更");
                         QAction* action_edge      = contextMenu.addAction("端変更");
                         contextMenu.addSeparator();
                         QAction* action_init_text = contextMenu.addAction("文字位置初期化");
                         contextMenu.addSeparator();
                         QAction* action_del = contextMenu.addAction("削除");

                         // メニューを表示して選択されたアクションを取得
                         QAction* selectedAction =
                             contextMenu.exec(ui->tableWidget_create_dimension_list->viewport()->mapToGlobal(pos));

                         if (selectedAction == action_show) {
                             dimensionShow(true, false);
                         }
                         else if (selectedAction == action_hide) {
                             dimensionShow(false, false);
                         }
                         else if (selectedAction == action_color) {
                             dimensionColorChange();
                         }
                         else if (selectedAction == action_size) {
                             dimensionSizeChange();
                         }
                         else if (selectedAction == action_linewidth) {
                             dimensionLineWidthChange();
                         }
                         else if (selectedAction == action_edge) {
                             dimensionEdgeChange();
                         }
                         else if (selectedAction == action_dim_change) {
                             dimensionNameChange();
                         }
                         else if (selectedAction == action_dim_show) {
                             dimensionNameShow(true);
                         }
                         else if (selectedAction == action_dim_hide) {
                             dimensionNameShow(false);
                         }
                         else if (selectedAction == action_init_text) {
                             dimensionLineInitTextPos();
                         }
                         else if (selectedAction == action_del) {
                             removeDimension();
                         }
                     });

    connect(ui->tableWidget_create_dimension_list, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        if (column == DIMENSION_LIST_NAME) {
            return;
        }
        Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                         ->data(Qt::UserRole)
                         .toULongLong();

        QCheckBox* check_box = (QCheckBox*)ui->tableWidget_create_dimension_list->cellWidget(row, DIMENSION_LIST_VIEW)
                                   ->findChild<QCheckBox*>();
        if (!check_box->isChecked()) {
            check_box->blockSignals(true);
            check_box->setChecked(true);
            node->show();
            check_box->blockSignals(false);
        }

        ui->obj3dViewer->fitDisplay(node);
    });

    connect(ui->tableWidget_create_dimension_list, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (item && item->column() == DIMENSION_LIST_NAME) {
            int row = item->row();
            if (row < ui->tableWidget_create_dimension_list->rowCount()) {
                auto row_item = ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM);
                if (row_item) {
                    Node*      node    = (Node*)row_item->data(Qt::UserRole).toULongLong();
                    Dimension* dim_obj = node->object<Dimension>();

                    bool change = false;

                    const auto& new_name = item->text().toStdWString();
                    if (node->name() != new_name) {
                        node->setName(item->text().toStdWString());
                        dim_obj->setName(node->name());
                        change = true;
                    }

                    bool item_checked = (item->checkState() & Qt::Checked);
                    if (item_checked != dim_obj->dispName()) {
                        dim_obj->setDispName(item_checked);
                        change = true;
                    }

                    if (change) {
                        ui->obj3dViewer->update();
                    }
                }
            }
        }
    });

    ui->tableWidget_create_dimension_list->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->tableWidget_create_dimension_list->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void DimensionCtrl::createAutoDimensionList()
{
    QStringList labels;
    labels << "View"
           << "Name"
           << "Val"
           << "Mat"
           << "Col"
           << "Size"
           << "Width"
           << "Edge"
           << "配置"
           << "断面"
           << "X"
           << "Y"
           << "Z"
           << "X"
           << "Y"
           << "Z";

    ui->tableWidget_create_auto_dimension->setColumnCount(labels.size());
    ui->tableWidget_create_auto_dimension->setHorizontalHeaderLabels(labels);
    // ui->tableWidget_create_auto_dimension->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QVector<int> columnMargins = {15, 25, 10, 10, 15, 20, 10, 20,
                                  25, 10, 43, 43, 43, 43, 43, 43};    // 各列に対する余白の設定

    QFontMetrics font_metrics(ui->tableWidget_create_auto_dimension->horizontalHeader()->font());
    for (int i = 0; i < labels.size(); ++i) {
        int size = font_metrics.horizontalAdvance(labels[i]);

        ui->tableWidget_create_auto_dimension->setColumnWidth(i, size + columnMargins[i]);
    }

    /*
    /// スプリッターの位置調整(テーブルを大きくする)
    QTimer::singleShot(0, this, [this]() {
        QSplitter* splitter = (QSplitter*)ui->tableWidget_create_auto_dimension->parentWidget();
        QList<int> sizes    = splitter->sizes();
        if (sizes.size() == 2) {
            int size_0 = (sizes[0] + sizes[1]) / 3 * 2;
            int size_1 = sizes[0] + sizes[1] - size_0;
            splitter->setSizes({size_0, size_1});
        }
    });
    */
    /// スプリッターの位置調整(テーブルを大きくする)
    QTimer::singleShot(0, this, [this]() {
        QSplitter* splitter = (QSplitter*)ui->tableWidget_create_auto_dimension->parentWidget();
        QList<int> sizes    = splitter->sizes();
        if (sizes.size() == 2) {
            int size_0 = (sizes[0] + sizes[1]) / 3;
            int size_1 = sizes[0] + sizes[1] - size_0;
            splitter->setSizes({size_0, size_1});
        }
    });

    // カスタムコンテキストメニューを有効化
    ui->tableWidget_create_auto_dimension->setContextMenuPolicy(Qt::CustomContextMenu);

    // シグナルとスロットを接続
    QObject::connect(ui->tableWidget_create_auto_dimension, &QTableWidget::customContextMenuRequested,
                     [this](const QPoint& pos) {
                         // 右クリックメニューを作成
                         QMenu contextMenu;

                         // メニュー項目を追加
                         QAction* action_show = contextMenu.addAction("表示");
                         QAction* action_hide = contextMenu.addAction("非表示");
                         contextMenu.addSeparator();
                         QMenu    sub_value("測長値");
                         QAction* action_val_show = sub_value.addAction("表示");
                         QAction* action_val_hide = sub_value.addAction("非表示");
                         contextMenu.addMenu(&sub_value);
                         contextMenu.addSeparator();
                         QMenu    sub_material("材質名");
                         QAction* action_mat_show = sub_material.addAction("表示");
                         QAction* action_mat_hide = sub_material.addAction("非表示");
                         contextMenu.addMenu(&sub_material);
                         contextMenu.addSeparator();
                         QMenu    sub_dimension_name("寸法名");
                         QAction* action_dim_show   = sub_dimension_name.addAction("表示");
                         QAction* action_dim_hide   = sub_dimension_name.addAction("非表示");
                         QAction* action_dim_change = sub_dimension_name.addAction("変更");
                         contextMenu.addMenu(&sub_dimension_name);
                         contextMenu.addSeparator();
                         QAction* action_color     = contextMenu.addAction("色変更");
                         QAction* action_size      = contextMenu.addAction("サイズ変更");
                         QAction* action_linewidth = contextMenu.addAction("線幅変更");
                         QAction* action_edge      = contextMenu.addAction("端変更");
                         contextMenu.addSeparator();
                         QAction* action_text_align = contextMenu.addAction("文字配置");
                         QAction* action_init_text  = contextMenu.addAction("文字位置初期化");
                         contextMenu.addSeparator();
                         QAction* action_reverse = contextMenu.addAction("順番反転");
                         contextMenu.addSeparator();
                         QMenu    sub_on_plane("断面追従");
                         QAction* action_on_plane_on  = sub_on_plane.addAction("ON");
                         QAction* action_on_plane_off = sub_on_plane.addAction("OFF");
                         contextMenu.addMenu(&sub_on_plane);
                         contextMenu.addSeparator();
                         QAction* action_del = contextMenu.addAction("削除");

                         // メニューを表示して選択されたアクションを取得
                         QAction* selectedAction =
                             contextMenu.exec(ui->tableWidget_create_auto_dimension->viewport()->mapToGlobal(pos));

                         if (selectedAction == action_show) {
                             autoDimensionShow(true, false);
                         }
                         else if (selectedAction == action_hide) {
                             autoDimensionShow(false, false);
                         }
                         else if (selectedAction == action_val_show) {
                             autoDimensionValueShow(true);
                         }
                         else if (selectedAction == action_val_hide) {
                             autoDimensionValueShow(false);
                         }
                         else if (selectedAction == action_mat_show) {
                             autoDimensionMaterialShow(true);
                         }
                         else if (selectedAction == action_mat_hide) {
                             autoDimensionMaterialShow(false);
                         }
                         else if (selectedAction == action_dim_change) {
                             autoDimensionNameChange();
                         }
                         else if (selectedAction == action_dim_show) {
                             autoDimensionNameShow(true);
                         }
                         else if (selectedAction == action_dim_hide) {
                             autoDimensionNameShow(false);
                         }
                         else if (selectedAction == action_color) {
                             autoDimensionColorChange();
                         }
                         else if (selectedAction == action_size) {
                             autoDimensionSizeChange();
                         }
                         else if (selectedAction == action_linewidth) {
                             autoDimensionLineWidthChange();
                         }
                         else if (selectedAction == action_edge) {
                             autoDimensionEdgeChange();
                         }
                         else if (selectedAction == action_text_align) {
                             autoDimensionTextAlignChange();
                         }
                         else if (selectedAction == action_init_text) {
                             autoDimensionLineInitTextPos();
                         }
                         else if (selectedAction == action_reverse) {
                             autoDimensionReverse();
                         }
                         else if (selectedAction == action_on_plane_on) {
                             autoDimensionOnPlane(true);
                         }
                         else if (selectedAction == action_on_plane_off) {
                             autoDimensionOnPlane(false);
                         }
                         else if (selectedAction == action_del) {
                             removeAutoDimension();
                         }
                     });

    connect(ui->tableWidget_create_auto_dimension, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        if (column == AUTO_DIMENSION_LIST_NAME) {
            return;
        }
        Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                         ->data(Qt::UserRole)
                         .toULongLong();

        QCheckBox* check_box =
            (QCheckBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_VIEW)
                ->findChild<QCheckBox*>();
        if (!check_box->isChecked()) {
            check_box->blockSignals(true);
            check_box->setChecked(true);
            node->show();
            check_box->blockSignals(false);
        }

        ui->obj3dViewer->fitDisplay(node);
    });

    // 行選択時の動作を設定
    connect(ui->tableWidget_create_auto_dimension->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() {
                auto index = ui->tableWidget_create_auto_dimension->currentIndex();
                if (index.isValid()) {
                    Node* node =
                        (Node*)ui->tableWidget_create_auto_dimension->item(index.row(), AUTO_DIMENSION_NODE_ITEM)
                            ->data(Qt::UserRole)
                            .toULongLong();
                    m_curent_text_auto_dimension = node;
                    updateAutoDimensionTextList();
                }
                else {
                    m_curent_text_auto_dimension = nullptr;
                    updateAutoDimensionTextList();
                }
            });

    connect(ui->tableWidget_create_auto_dimension, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (item && item->column() == DIMENSION_LIST_NAME) {
            int row = item->row();
            if (row < ui->tableWidget_create_auto_dimension->rowCount()) {
                auto row_item = ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM);
                if (row_item) {
                    Node*           node    = (Node*)row_item->data(Qt::UserRole).toULongLong();
                    MultiDimension* dim_obj = node->object<MultiDimension>();

                    bool change = false;

                    const auto& new_name = item->text().toStdWString();
                    if (node->name() != new_name) {
                        node->setName(item->text().toStdWString());
                        dim_obj->setName(node->name());
                        change = true;
                    }

                    bool item_checked = (item->checkState() & Qt::Checked);
                    if (item_checked != dim_obj->dispName()) {
                        dim_obj->setDispName(item_checked);
                        change = true;
                    }

                    if (change) {
                        ui->obj3dViewer->update();
                    }
                }
            }
        }
    });

    ui->tableWidget_create_auto_dimension->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->tableWidget_create_auto_dimension->setSelectionBehavior(QAbstractItemView::SelectRows);
}

class CustomAutoDimensionTextMenu : public QMenu {
public:
    CustomAutoDimensionTextMenu(DimensionCtrl* ctrl) : QMenu(nullptr), m_ctrl(ctrl) {}

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if ((event->key() == Qt::Key_C) && (event->modifiers() & Qt::ControlModifier)) {
            m_ctrl->autoDimensionTextCopy();
            close();
        }
        else if ((event->key() == Qt::Key_A) && (event->modifiers() & Qt::ControlModifier)) {
            m_ctrl->autoDimensionTextAllSelect();
            close();
        }
        else {
            QMenu::keyPressEvent(event);
        }
    }
    DimensionCtrl* m_ctrl;
};

void DimensionCtrl::createAutoDimensionText()
{
    QStringList labels;
    labels << "No."
           << "Material"
           << "Value"
           << "Material (original)";

    ui->tableWidget_create_auto_dimension_text->setColumnCount(labels.size());
    ui->tableWidget_create_auto_dimension_text->setHorizontalHeaderLabels(labels);

    QVector<int> columnMargins = {20, 150, 40, 100};    // 各列に対する余白の設定

    QFontMetrics font_metrics(ui->tableWidget_create_auto_dimension_text->horizontalHeader()->font());
    for (int i = 0; i < labels.size(); ++i) {
        int size = font_metrics.horizontalAdvance(labels[i]);

        ui->tableWidget_create_auto_dimension_text->setColumnWidth(i, size + columnMargins[i]);
    }

    // カスタムコンテキストメニューを有効化
    ui->tableWidget_create_auto_dimension_text->setContextMenuPolicy(Qt::CustomContextMenu);

    // シグナルとスロットを接続
    QObject::connect(ui->tableWidget_create_auto_dimension_text, &QTableWidget::customContextMenuRequested,
                     [this](const QPoint& pos) {
                         // 右クリックメニューを作成
                         CustomAutoDimensionTextMenu contextMenu(this);

                         QAction* action_material_change = contextMenu.addAction("Change Material");
                         QAction* action_material_clear  = contextMenu.addAction("Clear Material Change");
                         contextMenu.addSeparator();
                         QAction* action_all_select = contextMenu.addAction("All Select    (Ctrl+A)");
                         contextMenu.addSeparator();
                         QAction* action_copy     = contextMenu.addAction("Copy           (Ctrl+C)");
                         QAction* action_all_copy = contextMenu.addAction("Copy All");
                         QAction* action_all_copy_except_org =
                             contextMenu.addAction("Copy All except Material(original)");

                         QAction* selectedAction =
                             contextMenu.exec(ui->tableWidget_create_auto_dimension_text->viewport()->mapToGlobal(pos));

                         if (selectedAction == action_all_select) {
                             autoDimensionTextAllSelect();
                         }
                         else if (selectedAction == action_material_change) {
                             autoDimensionTextNameChange();
                         }
                         else if (selectedAction == action_material_clear) {
                             clearAutoDimensionMaterialNameChange(true);
                         }
                         else if (selectedAction == action_copy) {
                             autoDimensionTextCopy();
                         }
                         else if (selectedAction == action_all_copy) {
                             autoDimensionTextAllCopy();
                         }
                         else if (selectedAction == action_all_copy_except_org) {
                             autoDimensionTextAllCopyExceptOrg();
                         }
                     });

    connect(
        ui->tableWidget_create_auto_dimension_text, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
            if (m_curent_text_auto_dimension == nullptr) {
                return;
            }

            if (item && item->column() == 1) {
                MultiDimension* multi_dimension = (MultiDimension*)m_curent_text_auto_dimension->object();
                int             count           = multi_dimension->count();

                int row = item->row();
                if (row < ui->tableWidget_create_auto_dimension_text->rowCount() && row < count) {
                    auto row_item = ui->tableWidget_create_auto_dimension_text->item(row, 1);
                    if (row_item) {
                        auto& current_change = m_auto_dimension_material_change[multi_dimension->materialOriginal(row)];
                        const auto item_text = item->text().toStdWString();

                        if (current_change != item_text) {
                            current_change = item_text;

                            /// ※単体でなく全部に適用
                            updateAutoDimensionMaterialNameChange();
                        }
                    }
                }
            }
        });

    ui->tableWidget_create_auto_dimension_text->setEditTriggers(QAbstractItemView::DoubleClicked);

    /// 初期リスト
    /*
    for (int ic = 0; ic < 20; ++ic) {
        addAutoDimensionText(nullptr, ic);
    }
    */
}

void DimensionCtrl::clearAutoDimensionText()
{
    ui->tableWidget_create_auto_dimension_text->setRowCount(0);    /// 処理遅くないので全クリアにする
    /*
    int row_count = ui->tableWidget_create_auto_dimension_text->rowCount();
    for (int ic = 0; ic < row_count; ++ic) {
        addAutoDimensionText(nullptr, ic);
    }
    */
}

void DimensionCtrl::createAutoDimensionGUIUpdate()
{
    if (!isAutoDimension1Pick()) {
        ui->label_create_auto_dimension_pos1->setEnabled(false);
        ui->label_create_auto_dimension_pos_1_x->setEnabled(false);
        ui->label_create_auto_dimension_pos_1_y->setEnabled(false);
        ui->label_create_auto_dimension_pos_1_z->setEnabled(false);
        ui->doubleSpinBox_create_auto_dimension_pos1_x->setEnabled(false);
        ui->doubleSpinBox_create_auto_dimension_pos1_y->setEnabled(false);
        ui->doubleSpinBox_create_auto_dimension_pos1_z->setEnabled(false);
        ui->pushButton_create_auto_dimension_pos_preview->setEnabled(false);
        ui->pushButton_create_auto_dimension_pos_create->setEnabled(false);
        ui->checkBox_create_auto_dimension_continue->setEnabled(true);
        auto type = createAutoDimensionType();
        switch (type) {
            case CreateDimensionType::X:
            case CreateDimensionType::Y:
            case CreateDimensionType::Z:
            case CreateDimensionType::XYZ:
                ui->checkBox_create_auto_dimension_along_plane->setEnabled(false);
                break;
            default:
                ui->checkBox_create_auto_dimension_along_plane->setEnabled(true);
                break;
        }
        clearTemporaryPreviewAutoDimension();
    }
    else {
        ui->obj3dViewer->pickClear();
        ui->label_create_auto_dimension_pos1->setEnabled(true);
        ui->label_create_auto_dimension_pos_1_x->setEnabled(true);
        ui->label_create_auto_dimension_pos_1_y->setEnabled(true);
        ui->label_create_auto_dimension_pos_1_z->setEnabled(true);
        ui->doubleSpinBox_create_auto_dimension_pos1_x->setEnabled(true);
        ui->doubleSpinBox_create_auto_dimension_pos1_y->setEnabled(true);
        ui->doubleSpinBox_create_auto_dimension_pos1_z->setEnabled(true);
        ui->pushButton_create_auto_dimension_pos_preview->setEnabled(true);
        ui->pushButton_create_auto_dimension_pos_create->setEnabled(true);
        ui->checkBox_create_auto_dimension_continue->setEnabled(false);
        ui->checkBox_create_auto_dimension_along_plane->setEnabled(false);

        if (ui->pushButton_create_auto_dimension_pos_preview->isChecked()) {
            createAutoDimensionByPoint(true);
        }
    }
}

bool DimensionCtrl::isCreateDimension2Points() const
{
    return ui->pushButton_create_dimension->isChecked();
}

bool DimensionCtrl::isCreateAutoDimension() const
{
    return ui->pushButton_create_auto_dimension->isChecked();
}

bool DimensionCtrl::isCreateAutoDimension2Points() const
{
    if (isCreateAutoDimension() && !isAutoDimension1Pick()) {
        return true;
    }
    return false;
}

int DimensionCtrl::dimensionStrSize() const
{
    return ui->spinBox_create_dimension_size->value();
}

int DimensionCtrl::autoDimensionStrSize() const
{
    return ui->spinBox_create_auto_dimension_size->value();
}

PickSnap DimensionCtrl::dimensionSnap() const
{
    if (ui->checkBox_create_dimension_snap->isChecked()) {
        const auto& snap = ui->comboBox_ceate_dimension_snap->currentText();
        if (snap == "Vertex") {
            return PickSnap::SnapVertex;
        }
        if (snap == "Edge") {
            return PickSnap::SnapEdge;
        }
        if (snap == "Vertex+Edge") {
            return PickSnap::SnapVertexEdge;
        }
    }

    return PickSnap::SnapNone;
}

Dimension::Type DimensionCtrl::dimensionType() const
{
    const QString& dimension_type = ui->comboBox_create_dimension_type->currentText();
    if (dimension_type == QString("X")) {
        return Dimension::Type::XDistance;
    }
    else if (dimension_type == QString("Y")) {
        return Dimension::Type::YDistance;
    }
    else if (dimension_type == QString("Z")) {
        return Dimension::Type::ZDistance;
    }

    return Dimension::Type::XYZDistance;
}

DimensionCtrl::CreateDimensionType DimensionCtrl::createDimensionType(const QString& dimension_type) const
{
    if (dimension_type == QString("2 Points")) {
        if (m_ctrl_release) {
            return CreateDimensionType::Min;
        }
        if (m_ctrl_press) {
            return CreateDimensionType::VHD;
        }
        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
            return CreateDimensionType::VHD;
        }
        return CreateDimensionType::Min;
    }
    else if (dimension_type == QString("X")) {
        return CreateDimensionType::X;
    }
    else if (dimension_type == QString("Y")) {
        return CreateDimensionType::Y;
    }
    else if (dimension_type == QString("Z")) {
        return CreateDimensionType::Z;
    }
    else if (dimension_type == QString("X+Y+Z")) {
        return CreateDimensionType::XYZ;
    }
    else if (dimension_type == QString("Vertical")) {
        return CreateDimensionType::Vertical;
    }
    else if (dimension_type == QString("Horizontal")) {
        return CreateDimensionType::Horizontal;
    }
    else if (dimension_type == QString("Diagonal")) {
        return CreateDimensionType::Diagonal;
    }
    else if (dimension_type == QString("V+H+D")) {
        return CreateDimensionType::VHD;
    }

    assert(0);
    return CreateDimensionType::XYZ;
}

DimensionCtrl::CreateDimensionType DimensionCtrl::createDimensionType() const
{
    const QString& dimension_type = ui->comboBox_create_dimension_type->currentText();
    return createDimensionType(dimension_type);
}

bool DimensionCtrl::isXYZDimension(CreateDimensionType type) const
{
    switch (type) {
        case CreateDimensionType::X:
        case CreateDimensionType::Y:
        case CreateDimensionType::Z:
        case CreateDimensionType::XYZ:
            return true;
    }

    return false;
}

DimensionCtrl::CreateDimensionType DimensionCtrl::createAutoDimensionType() const
{
    const QString& dimension_type = ui->comboBox_create_auto_dimension_type->currentText();
    return createDimensionType(dimension_type);
}

bool DimensionCtrl::isAutoDimension1Pick() const
{
    if (ui->checkBox_create_auto_dimension_extend->isChecked()) {
        auto type = createAutoDimensionType();
        switch (type) {
            case CreateDimensionType::X:
            case CreateDimensionType::Y:
            case CreateDimensionType::Z:
            case CreateDimensionType::Vertical:
            case CreateDimensionType::Horizontal:
                return true;
        }
    }

    return false;
}

void DimensionCtrl::clearAutoDimension()
{
    m_auto_dim_count = 0;
    // ui->textEdit_create_auto_dimension->setText(QString());
    clearAutoDimensionText();
    ui->tableWidget_create_auto_dimension->setRowCount(0);
    m_curent_text_auto_dimension = nullptr;
}

void DimensionCtrl::clearDimensions()
{
    m_dim_count = 0;
    ui->tableWidget_create_dimension_list->setRowCount(0);
}

void DimensionCtrl::addDimension(Node* dimension)
{
    auto dimension_object = (Dimension*)dimension->object();

    dimension->setName(QString("D%1").arg(++m_dim_count, 3, 10, QChar('0')));
    dimension_object->setName(dimension->name());

    auto table_widget = ui->tableWidget_create_dimension_list;

    int row = table_widget->rowCount();
    table_widget->insertRow(row);

    int column = 0;

    /// View
    QWidget*     checkBoxWidget = new QWidget();
    QCheckBox*   checkBox       = new QCheckBox(checkBoxWidget);
    QHBoxLayout* layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);
    checkBox->setChecked(true);
    connect(checkBox, &QCheckBox::toggled, this, [this, dimension](bool checked) {
        if (checked) {
            dimension->show();
            ui->obj3dViewer->update();
        }
        else {
            dimension->hide();
            ui->obj3dViewer->update();
        }
    });

    /// Name
    QTableWidgetItem* noItem = new QTableWidgetItem(QString::fromStdWString(dimension->name()));
    noItem->setFlags(noItem->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    noItem->setCheckState(ui->checkBox_create_dimension_disp_name->isChecked() ? Qt::Checked : Qt::Unchecked);
    table_widget->setItem(row, column++, noItem);
    dimension_object->setDispName(ui->checkBox_create_dimension_disp_name->isChecked());

    /// Distance
    QTableWidgetItem* distanceItem = new QTableWidgetItem(QString::fromStdWString(dimension_object->createText(true)));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    /// Color
    QPushButton* colorButton = new QPushButton();
    table_widget->setCellWidget(row, column++, colorButton);

    /// ボタンの背景色を選択した色に設定
    auto   color = dimension_object->color();
    QColor qcolor;
    qcolor.setRedF(color[0]);
    qcolor.setGreenF(color[1]);
    qcolor.setBlueF(color[2]);
    colorButton->setStyleSheet(Vox3DForm::colorStyle(qcolor));
    connect(colorButton, &QPushButton::clicked, this, [this, colorButton, dimension_object]() {
        auto   init_color = dimension_object->color();
        QColor init_qcolor;
        init_qcolor.setRedF(init_color[0]);
        init_qcolor.setGreenF(init_color[1]);
        init_qcolor.setBlueF(init_color[2]);

        QColor selectedColor = QColorDialog::getColor(init_qcolor, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            // ボタンの背景色を選択した色に設定
            colorButton->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            dimension_object->setColor(selectedColor.redF(), selectedColor.greenF(), selectedColor.blueF());
            ui->obj3dViewer->update();
        }
    });

    /// "Size"
    CustomSpin* size_button = new CustomSpin();
    size_button->setValue(dimension_object->strSize());
    size_button->setRange(1, 999);
    size_button->setContextMenuPolicy(Qt::NoContextMenu);
    table_widget->setCellWidget(row, column++, size_button);
    connect(size_button, &QSpinBox::valueChanged, this, [this, dimension_object](int value) {
        dimension_object->setStrSize(value);
        ui->obj3dViewer->update();
    });

    /// "Line Width"
    CustomSpin* line_width_button = new CustomSpin();
    line_width_button->setValue(dimension_object->lineWidth());
    line_width_button->setRange(1, 20);
    line_width_button->setContextMenuPolicy(Qt::NoContextMenu);
    table_widget->setCellWidget(row, column++, line_width_button);
    connect(line_width_button, &QSpinBox::valueChanged, this, [this, dimension_object](int value) {
        dimension_object->setLineWidth(value);
        ui->obj3dViewer->update();
    });

    /// Edge
    CustomCombo* edge_combo_box = new CustomCombo();
    for (int ic = 0; ic < ui->comboBox_create_dimension_edge_type->count(); ++ic) {
        edge_combo_box->addItem(ui->comboBox_create_dimension_edge_type->itemText(ic));
    }
    edge_combo_box->setCurrentIndex(ui->comboBox_create_dimension_edge_type->currentIndex());
    table_widget->setCellWidget(row, column++, edge_combo_box);
    connect(edge_combo_box, &QComboBox::currentIndexChanged, this, [this, dimension_object](int index) {
        dimension_object->setEdgeType((Dimension::EdgeType)index);
        ui->obj3dViewer->update();
    });
    dimension_object->setEdgeType((Dimension::EdgeType)ui->comboBox_create_dimension_edge_type->currentIndex());

    /// "X, Y, Z" 列: 数値（ReadOnly）
    const auto& pos_start = dimension_object->posStart();
    const auto& pos_end   = dimension_object->posEnd();

    distanceItem = new QTableWidgetItem(QString::number(pos_start.x() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);
    distanceItem->setData(Qt::UserRole, reinterpret_cast<qulonglong>(dimension));    /// 暫定 - Xposで持つ

    distanceItem = new QTableWidgetItem(QString::number(pos_start.y() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_start.z() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_end.x() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_end.y() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_end.z() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);
}

void DimensionCtrl::addAutoDimension(Node* dimension, int insert_row, bool set_new_name)
{
    auto dimension_object = (MultiDimension*)dimension->object();

    if (set_new_name) {
        dimension->setName(QString("A%1").arg(++m_auto_dim_count, 3, 10, QChar('0')));
        dimension_object->setName(dimension->name());
    }

    dimension_object->setDispName(true);

    auto table_widget = ui->tableWidget_create_auto_dimension;

    int row = table_widget->rowCount();
    if (insert_row >= 0) {
        row = insert_row;
    }
    table_widget->insertRow(row);

    int column = 0;

    /// View
    QWidget*     checkBoxWidget = new QWidget();
    QCheckBox*   checkBox       = new QCheckBox(checkBoxWidget);
    QHBoxLayout* layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);
    checkBox->setChecked(true);
    connect(checkBox, &QCheckBox::toggled, this, [this, dimension](bool checked) {
        if (checked) {
            dimension->show();
            ui->obj3dViewer->update();
        }
        else {
            dimension->hide();
            ui->obj3dViewer->update();
        }
    });

    /// Name
    QTableWidgetItem* noItem = new QTableWidgetItem(QString::fromStdWString(dimension->name()));
    noItem->setFlags(noItem->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    noItem->setCheckState(ui->checkBox_create_auto_dimension_disp_name->isChecked() ? Qt::Checked : Qt::Unchecked);
    table_widget->setItem(row, column++, noItem);
    dimension_object->setDispName(ui->checkBox_create_auto_dimension_disp_name->isChecked());

    /// Value
    checkBoxWidget = new QWidget();
    checkBox       = new QCheckBox(checkBoxWidget);
    layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);
    checkBox->setChecked(dimension_object->isDisplayValue());

    connect(checkBox, &QCheckBox::toggled, this, [this, dimension](bool checked) {
        auto dimension_object = (MultiDimension*)dimension->object();
        dimension_object->setDisplayValue(checked);
        dimension_object->updateText();
        ui->obj3dViewer->update();
    });

    /// Material
    checkBoxWidget = new QWidget();
    checkBox       = new QCheckBox(checkBoxWidget);
    layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);
    checkBox->setChecked(dimension_object->isDisplayMaterial());

    connect(checkBox, &QCheckBox::toggled, this, [this, dimension](bool checked) {
        auto dimension_object = (MultiDimension*)dimension->object();
        dimension_object->setDisplayMaterial(checked);
        dimension_object->updateText();
        ui->obj3dViewer->update();
    });

    /// Color
    QPushButton* colorButton = new QPushButton();
    table_widget->setCellWidget(row, column++, colorButton);

    /// ボタンの背景色を選択した色に設定
    auto   color = dimension_object->color();
    QColor qcolor;
    qcolor.setRedF(color[0]);
    qcolor.setGreenF(color[1]);
    qcolor.setBlueF(color[2]);
    colorButton->setStyleSheet(Vox3DForm::colorStyle(qcolor));
    connect(colorButton, &QPushButton::clicked, this, [this, colorButton, dimension]() {
        auto   dimension_object = (MultiDimension*)dimension->object();
        auto   init_color       = dimension_object->color();
        QColor init_qcolor;
        init_qcolor.setRedF(init_color[0]);
        init_qcolor.setGreenF(init_color[1]);
        init_qcolor.setBlueF(init_color[2]);

        QColor selectedColor = QColorDialog::getColor(init_qcolor, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            // ボタンの背景色を選択した色に設定
            colorButton->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            dimension_object->setColor(selectedColor.redF(), selectedColor.greenF(), selectedColor.blueF());
            ui->obj3dViewer->update();
        }
    });

    /// "Size"
    CustomSpin* size_button = new CustomSpin();
    size_button->setValue(dimension_object->strSize());
    size_button->setRange(1, 999);
    size_button->setContextMenuPolicy(Qt::NoContextMenu);
    table_widget->setCellWidget(row, column++, size_button);
    connect(size_button, &QSpinBox::valueChanged, this, [this, dimension](int value) {
        auto dimension_object = (MultiDimension*)dimension->object();
        dimension_object->setStrSize(value);
        ui->obj3dViewer->update();
    });

    /// "Line Width"
    CustomSpin* line_width_button = new CustomSpin();
    line_width_button->setValue(dimension_object->lineWidth());
    line_width_button->setRange(1, 20);
    line_width_button->setContextMenuPolicy(Qt::NoContextMenu);
    table_widget->setCellWidget(row, column++, line_width_button);
    connect(line_width_button, &QSpinBox::valueChanged, this, [this, dimension](int value) {
        auto dimension_object = (MultiDimension*)dimension->object();
        dimension_object->setLineWidth(value);
        ui->obj3dViewer->update();
    });

    /// Edge
    CustomCombo* edge_combo_box = new CustomCombo();
    for (int ic = 0; ic < ui->comboBox_create_auto_dimension_edge_type->count(); ++ic) {
        edge_combo_box->addItem(ui->comboBox_create_auto_dimension_edge_type->itemText(ic));
    }
    edge_combo_box->setCurrentIndex(ui->comboBox_create_auto_dimension_edge_type->currentIndex());
    table_widget->setCellWidget(row, column++, edge_combo_box);
    connect(edge_combo_box, &QComboBox::currentIndexChanged, this, [this, dimension](int index) {
        auto dimension_object = (MultiDimension*)dimension->object();
        dimension_object->setEdgeType((Dimension::EdgeType)index);
        ui->obj3dViewer->update();
    });
    dimension_object->setEdgeType((Dimension::EdgeType)ui->comboBox_create_auto_dimension_edge_type->currentIndex());

    /// 配置
    CustomCombo* combobox = new CustomCombo();
    table_widget->setCellWidget(row, column++, combobox);
    combobox->addItems({"Auto", "H", "V"});
    switch (dimension_object->textAlign()) {
        case Dimension::TextAlign::Auto:
            combobox->setCurrentText("Auto");
            break;
        case Dimension::TextAlign::Horizontal:
            combobox->setCurrentText("H");
            break;
        case Dimension::TextAlign::Vertical:
            combobox->setCurrentText("V");
            break;
        default:
            break;
    }
    connect(combobox, &QComboBox::currentTextChanged, this, [this, combobox, dimension](const QString& text) {
        auto dimension_object = (MultiDimension*)dimension->object();
        dimension_object->setTextAlign(autoDimensionTextAlign(combobox));
        ui->obj3dViewer->update();
    });

    /// 断面
    checkBoxWidget = new QWidget();
    checkBox       = new QCheckBox(checkBoxWidget);
    layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);
    checkBox->setChecked(dimension_object->isOnSectionPlane());

    connect(checkBox, &QCheckBox::toggled, this, [this, dimension](bool checked) {
        auto dimension_object = (MultiDimension*)dimension->object();
        dimension_object->setOnSectionPlane(checked);
        ui->obj3dViewer->update();
    });

    /// "X, Y, Z" 列: 数値（ReadOnly）
    const auto& pos_start = dimension_object->posStart();
    const auto& pos_end   = dimension_object->posEnd();

    QTableWidgetItem* distanceItem = new QTableWidgetItem(QString::number(pos_start.x() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);
    distanceItem->setData(Qt::UserRole, reinterpret_cast<qulonglong>(dimension));    /// 暫定

    distanceItem = new QTableWidgetItem(QString::number(pos_start.y() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_start.z() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_end.x() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_end.y() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    distanceItem = new QTableWidgetItem(QString::number(pos_end.z() * 1e3f, 'f', 2));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);

    table_widget->selectRow(row);
}

void DimensionCtrl::updateAutoDimensions()
{
    bool update_text_list = false;
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                         ->data(Qt::UserRole)
                         .toULongLong();
        MultiDimension* dimension = (MultiDimension*)node->object();
        auto            pos_start = dimension->posStart();
        auto            pos_end   = dimension->posEnd();
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_X)
            ->setText(QString::number(pos_start.x() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_Y)
            ->setText(QString::number(pos_start.y() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_Z)
            ->setText(QString::number(pos_start.z() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_X)
            ->setText(QString::number(pos_end.x() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_Y)
            ->setText(QString::number(pos_end.y() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_Z)
            ->setText(QString::number(pos_end.z() * 1e3f, 'f', 2));

        if (m_curent_text_auto_dimension.ptr() == node) {
            update_text_list = true;
        }
    }

    if (update_text_list) {
        updateAutoDimensionTextList();
    }
}

void DimensionCtrl::updateAutoDimension(Node* target)
{
    bool update_text_list = false;
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                         ->data(Qt::UserRole)
                         .toULongLong();
        if (node != target) {
            continue;
        }

        MultiDimension* dimension = (MultiDimension*)node->object();
        auto            pos_start = dimension->posStart();
        auto            pos_end   = dimension->posEnd();
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_X)
            ->setText(QString::number(pos_start.x() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_Y)
            ->setText(QString::number(pos_start.y() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_Z)
            ->setText(QString::number(pos_start.z() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_X)
            ->setText(QString::number(pos_end.x() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_Y)
            ->setText(QString::number(pos_end.y() * 1e3f, 'f', 2));
        ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_Z)
            ->setText(QString::number(pos_end.z() * 1e3f, 'f', 2));

        if (m_curent_text_auto_dimension.ptr() == node) {
            update_text_list = true;
        }

        break;
    }

    if (update_text_list) {
        updateAutoDimensionTextList();
    }
}

void DimensionCtrl::reset(bool delete_dimension)
{
    if (delete_dimension) {
        removeDimension(true);
        removeAutoDimension(true);
    }

    modeClear();

    auto clipping_ctrl = m_3DForm->clippingCtrl();

    float step_size;
    float cell_size = 1.0f;
    clipping_ctrl->boxAndStepSize(step_size, cell_size);

    ui->doubleSpinBox_create_auto_dimension_pos1_x->setSingleStep(step_size * cell_size * 1e3f);
    ui->doubleSpinBox_create_auto_dimension_pos1_y->setSingleStep(step_size * cell_size * 1e3f);
    ui->doubleSpinBox_create_auto_dimension_pos1_z->setSingleStep(step_size * cell_size * 1e3f);
}

void DimensionCtrl::modeClear()
{
    ui->obj3dViewer->setPickMode(false, dimensionSnap());
    ui->pushButton_create_dimension->setChecked(false);
    ui->pushButton_create_auto_dimension->setChecked(false);
    clearTemporaryPreviewAutoDimension();
}

void DimensionCtrl::dimensionShow(bool show, bool all)
{
    std::vector<QCheckBox*> buttons;
    QItemSelectionModel*    selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            if (show)
                node->show();
            else
                node->hide();

            buttons.emplace_back((QCheckBox*)ui->tableWidget_create_dimension_list->cellWidget(row, DIMENSION_LIST_VIEW)
                                     ->findChild<QCheckBox*>());
        }
    }

    if (buttons.size() == 0) {
        return;
    }

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setChecked(show);
        button->blockSignals(false);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::dimensionColorChange()
{
    std::vector<Dimension*>   dimensions;
    std::vector<QPushButton*> color_buttons;
    QItemSelectionModel*      selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((Dimension*)node->object());
            color_buttons.emplace_back(
                (QPushButton*)ui->tableWidget_create_dimension_list->cellWidget(row, DIMENSION_LIST_COL));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    auto   init_color = dimensions[0]->color();
    QColor init_qcolor;
    init_qcolor.setRedF(init_color[0]);
    init_qcolor.setGreenF(init_color[1]);
    init_qcolor.setBlueF(init_color[2]);

    QColor selectedColor = QColorDialog::getColor(init_qcolor, (QWidget*)this->parent(), "色を選択");
    if (!selectedColor.isValid()) {
        return;
    }

    QString style = Vox3DForm::colorStyle(selectedColor);
    for (auto& color_button : color_buttons) {
        color_button->setStyleSheet(style);
    }
    for (auto& dimension : dimensions) {
        dimension->setColor(selectedColor.redF(), selectedColor.greenF(), selectedColor.blueF());
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::dimensionSizeChange()
{
    std::vector<Dimension*> dimensions;
    std::vector<QSpinBox*>  buttons;
    QItemSelectionModel*    selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((Dimension*)node->object());
            buttons.emplace_back(
                (QSpinBox*)ui->tableWidget_create_dimension_list->cellWidget(row, DIMENSION_LIST_SIZE));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    SpinBoxDialog dlg("サイズ変更", (QWidget*)this->parent());
    dlg.m_spin->setValue(dimensions[0]->strSize());
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    int value = dlg.m_spin->value();

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setValue(value);
        button->blockSignals(false);
    }
    for (auto& dimension : dimensions) {
        dimension->setStrSize(value);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::dimensionLineWidthChange()
{
    std::vector<Dimension*> dimensions;
    std::vector<QSpinBox*>  buttons;
    QItemSelectionModel*    selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((Dimension*)node->object());
            buttons.emplace_back(
                (QSpinBox*)ui->tableWidget_create_dimension_list->cellWidget(row, DIMENSION_LIST_WIDTH));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    SpinBoxDialog dlg("線幅変更", (QWidget*)this->parent());
    dlg.m_spin->setValue(dimensions[0]->lineWidth());
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    int value = dlg.m_spin->value();

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setValue(value);
        button->blockSignals(false);
    }
    for (auto& dimension : dimensions) {
        dimension->setLineWidth(value);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::dimensionEdgeChange()
{
    std::vector<Dimension*> dimensions;
    std::vector<QComboBox*> buttons;
    QItemSelectionModel*    selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((Dimension*)node->object());
            buttons.emplace_back(
                (QComboBox*)ui->tableWidget_create_dimension_list->cellWidget(row, DIMENSION_LIST_EDGE));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    ComboBoxDialog dlg("端変更", (QWidget*)this->parent());

    for (int ic = 0; ic < ui->comboBox_create_dimension_edge_type->count(); ++ic) {
        dlg.m_combo->addItem(ui->comboBox_create_dimension_edge_type->itemText(ic));
    }
    dlg.m_combo->setCurrentIndex(dimensions[0]->edgeType());

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    int value = dlg.m_combo->currentIndex();

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setCurrentIndex(value);
        button->blockSignals(false);
    }
    for (auto& dimension : dimensions) {
        dimension->setEdgeType((Dimension::EdgeType)value);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::dimensionNameChange()
{
    std::vector<Node*>             dimensions;
    std::vector<QTableWidgetItem*> buttons;
    QItemSelectionModel*           selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back(node);
            buttons.emplace_back(
                (QTableWidgetItem*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_LIST_NAME));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    LineEditDialog dlg("寸法名 - 変更", (QWidget*)this->parent());

    const auto& name = QString::fromStdWString(dimensions[0]->name());
    dlg.m_edit->setText(name);
    for (int ic = 1; ic < dimensions.size(); ++ic) {
        if (dlg.m_edit->text() != QString::fromStdWString(dimensions[ic]->name())) {
            dlg.m_edit->setText("");
            break;
        }
    }

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const auto& text  = dlg.m_edit->text();
    const auto& wtext = dlg.m_edit->text().toStdWString();

    ui->tableWidget_create_dimension_list->blockSignals(true);
    for (auto& button : buttons) {
        button->setText(text);
    }
    ui->tableWidget_create_dimension_list->blockSignals(false);
    for (auto& dimension : dimensions) {
        dimension->setName(wtext);
        auto dim_obj = dimension->object<Dimension>();
        if (dim_obj) {
            dim_obj->setName(wtext);
        }
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::dimensionNameShow(bool show, bool all)
{
    std::vector<QTableWidgetItem*> buttons;
    QItemSelectionModel*           selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            Dimension* dim_obj = (Dimension*)node->object();
            dim_obj->setDispName(show);

            buttons.emplace_back(
                (QTableWidgetItem*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_LIST_NAME));
        }
    }

    if (buttons.size() == 0) {
        return;
    }

    ui->tableWidget_create_dimension_list->blockSignals(true);
    for (auto& button : buttons) {
        button->setCheckState(show ? Qt::Checked : Qt::Unchecked);
    }
    ui->tableWidget_create_dimension_list->blockSignals(false);

    ui->obj3dViewer->update();
}

void DimensionCtrl::dimensionLineInitTextPos()
{
    std::vector<Dimension*> dimensions;
    QItemSelectionModel*    selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            dimensions.emplace_back((Dimension*)node->object());
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    for (auto& dimension : dimensions) {
        dimension->initStrPosStart();
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::removeDimension(bool all)
{
    std::set<Node*> dimensions;

    QItemSelectionModel* selection_model = ui->tableWidget_create_dimension_list->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            dimensions.insert(node);
        }
    }

    if (all) {
        ui->tableWidget_create_dimension_list->setRowCount(0);
    }
    else {
        for (int row = ui->tableWidget_create_dimension_list->rowCount() - 1; row >= 0; row--) {
            if (selection_model->isRowSelected(row)) {
                ui->tableWidget_create_dimension_list->removeRow(row);
            }
        }
    }

    ui->obj3dViewer->sceneView()->sceneGraph()->rootNode()->removeChild(dimensions);
    ui->obj3dViewer->update();

    /// カウントゼロにする
    if (ui->tableWidget_create_dimension_list->rowCount() == 0) {
        m_dim_count = 0;
    }
}

int DimensionCtrl::removeAutoDimensionList(Node* node)
{
    int remove_row = -1;
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (node
            == (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                   ->data(Qt::UserRole)
                   .toULongLong()) {
            ui->tableWidget_create_auto_dimension->removeRow(row);
            remove_row = row;
            break;
        }
    }

    /// カウントゼロにする
    if (ui->tableWidget_create_auto_dimension->rowCount() == 0) {
        m_auto_dim_count = 0;
    }

    return remove_row;
}

void DimensionCtrl::autoDimensionShow(bool show, bool all)
{
    std::vector<QCheckBox*> buttons;
    QItemSelectionModel*    selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            if (show)
                node->show();
            else
                node->hide();

            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_VIEW)
                    ->findChild<QCheckBox*>());
        }
    }

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setChecked(show);
        button->blockSignals(false);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionValueShow(bool show)
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QCheckBox*>      buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            dimensions.emplace_back((MultiDimension*)node->object());
            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_VAL)
                    ->findChild<QCheckBox*>());
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    for (auto& dimension : dimensions) {
        dimension->setDisplayValue(show);
        dimension->updateText();
    }

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setChecked(show);
        button->blockSignals(false);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionMaterialShow(bool show)
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QCheckBox*>      buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            dimensions.emplace_back((MultiDimension*)node->object());
            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_MAT)
                    ->findChild<QCheckBox*>());
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    for (auto& dimension : dimensions) {
        dimension->setDisplayMaterial(show);
        dimension->updateText();
    }

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setChecked(show);
        button->blockSignals(false);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionNameChange()
{
    std::vector<Node*>             dimensions;
    std::vector<QTableWidgetItem*> buttons;
    QItemSelectionModel*           selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back(node);
            buttons.emplace_back(
                (QTableWidgetItem*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_NAME));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    LineEditDialog dlg("寸法名 - 変更", (QWidget*)this->parent());

    const auto& name = QString::fromStdWString(dimensions[0]->name());
    dlg.m_edit->setText(name);
    for (int ic = 1; ic < dimensions.size(); ++ic) {
        if (dlg.m_edit->text() != QString::fromStdWString(dimensions[ic]->name())) {
            dlg.m_edit->setText("");
            break;
        }
    }

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const auto& text  = dlg.m_edit->text();
    const auto& wtext = dlg.m_edit->text().toStdWString();

    ui->tableWidget_create_auto_dimension->blockSignals(true);
    for (auto& button : buttons) {
        button->setText(text);
    }
    ui->tableWidget_create_auto_dimension->blockSignals(false);
    for (auto& dimension : dimensions) {
        dimension->setName(wtext);
        auto dim_obj = dimension->object<MultiDimension>();
        if (dim_obj) {
            dim_obj->setName(wtext);
        }
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionNameShow(bool show, bool all)
{
    std::vector<QTableWidgetItem*> buttons;
    QItemSelectionModel*           selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            MultiDimension* dim_obj = (MultiDimension*)node->object();
            dim_obj->setDispName(show);

            buttons.emplace_back(
                (QTableWidgetItem*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_NAME));
        }
    }

    if (buttons.size() == 0) {
        return;
    }

    ui->tableWidget_create_auto_dimension->blockSignals(true);
    for (auto& button : buttons) {
        button->setCheckState(show ? Qt::Checked : Qt::Unchecked);
    }
    ui->tableWidget_create_auto_dimension->blockSignals(false);

    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionColorChange()
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QPushButton*>    color_buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((MultiDimension*)node->object());
            color_buttons.emplace_back(
                (QPushButton*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_COL));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    auto   init_color = dimensions[0]->color();
    QColor init_qcolor;
    init_qcolor.setRedF(init_color[0]);
    init_qcolor.setGreenF(init_color[1]);
    init_qcolor.setBlueF(init_color[2]);

    QColor selectedColor = QColorDialog::getColor(init_qcolor, (QWidget*)this->parent(), "色を選択");
    if (!selectedColor.isValid()) {
        return;
    }

    QString style = Vox3DForm::colorStyle(selectedColor);
    for (auto& color_button : color_buttons) {
        color_button->setStyleSheet(style);
    }
    for (auto& dimension : dimensions) {
        dimension->setColor(selectedColor.redF(), selectedColor.greenF(), selectedColor.blueF());
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionSizeChange()
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QSpinBox*>       buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((MultiDimension*)node->object());
            buttons.emplace_back(
                (QSpinBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_SIZE));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    SpinBoxDialog dlg("サイズ変更", (QWidget*)this->parent());
    dlg.m_spin->setValue(dimensions[0]->strSize());
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    int value = dlg.m_spin->value();

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setValue(value);
        button->blockSignals(false);
    }
    for (auto& dimension : dimensions) {
        dimension->setStrSize(value);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionLineWidthChange()
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QSpinBox*>       buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((MultiDimension*)node->object());
            buttons.emplace_back(
                (QSpinBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_WIDTH));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    SpinBoxDialog dlg("線幅変更", (QWidget*)this->parent());
    dlg.m_spin->setValue(dimensions[0]->lineWidth());
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    int value = dlg.m_spin->value();

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setValue(value);
        button->blockSignals(false);
    }
    for (auto& dimension : dimensions) {
        dimension->setLineWidth(value);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionEdgeChange()
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QComboBox*>      buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((MultiDimension*)node->object());
            buttons.emplace_back(
                (QComboBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_EDGE));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    ComboBoxDialog dlg("端変更", (QWidget*)this->parent());

    for (int ic = 0; ic < ui->comboBox_create_auto_dimension_edge_type->count(); ++ic) {
        dlg.m_combo->addItem(ui->comboBox_create_auto_dimension_edge_type->itemText(ic));
    }
    dlg.m_combo->setCurrentIndex(dimensions[0]->edgeType());

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    int value = dlg.m_combo->currentIndex();

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setCurrentIndex(value);
        button->blockSignals(false);
    }
    for (auto& dimension : dimensions) {
        dimension->setEdgeType((Dimension::EdgeType)value);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionTextAlignChange()
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QComboBox*>      buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((MultiDimension*)node->object());
            buttons.emplace_back(
                (QComboBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_ALIGN));
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    ComboBoxDialog dlg("文字配置", (QWidget*)this->parent());
    dlg.m_combo->addItems({"Auto", "H", "V"});
    switch (dimensions[0]->textAlign()) {
        case Dimension::TextAlign::Auto:
            dlg.m_combo->setCurrentText("Auto");
            break;
        case Dimension::TextAlign::Horizontal:
            dlg.m_combo->setCurrentText("H");
            break;
        case Dimension::TextAlign::Vertical:
            dlg.m_combo->setCurrentText("V");
            break;
        default:
            break;
    }
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const auto& current_text       = dlg.m_combo->currentText();
    auto        current_text_align = autoDimensionTextAlign(dlg.m_combo);

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setCurrentText(current_text);
        button->blockSignals(false);
    }
    for (auto& dimension : dimensions) {
        dimension->setTextAlign(current_text_align);
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionLineInitTextPos()
{
    std::vector<MultiDimension*> dimensions;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            dimensions.emplace_back((MultiDimension*)node->object());
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    for (auto& dimension : dimensions) {
        dimension->initStrPosStart();
    }
    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionReverse()
{
    // QTableWidgetItem* distanceItem = new QTableWidgetItem(QString::number(pos_start.x(), 'f', 3));
    // distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    // table_widget->setItem(row, column++, distanceItem);

    bool update_text_list = false;

    std::vector<MultiDimension*>   dimensions;
    std::vector<QTableWidgetItem*> buttons_x;
    std::vector<QTableWidgetItem*> buttons_y;
    std::vector<QTableWidgetItem*> buttons_z;
    std::vector<QTableWidgetItem*> buttons_xe;
    std::vector<QTableWidgetItem*> buttons_ye;
    std::vector<QTableWidgetItem*> buttons_ze;
    QItemSelectionModel*           selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();

            dimensions.emplace_back((MultiDimension*)node->object());
            buttons_x.emplace_back(ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_X));
            buttons_y.emplace_back(ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_Y));
            buttons_z.emplace_back(ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_START_Z));
            buttons_xe.emplace_back(ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_X));
            buttons_ye.emplace_back(ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_Y));
            buttons_ze.emplace_back(ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_LIST_END_Z));

            if (m_curent_text_auto_dimension.ptr() == node) {
                update_text_list = true;
            }
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    for (int ic = 0; ic < buttons_x.size(); ++ic) {
        dimensions[ic]->reverse();
        auto pos_start = dimensions[ic]->posStart();
        auto pos_end   = dimensions[ic]->posEnd();
        buttons_x[ic]->setText(QString::number(pos_start.x() * 1e3f, 'f', 2));
        buttons_y[ic]->setText(QString::number(pos_start.y() * 1e3f, 'f', 2));
        buttons_z[ic]->setText(QString::number(pos_start.z() * 1e3f, 'f', 2));
        buttons_xe[ic]->setText(QString::number(pos_end.x() * 1e3f, 'f', 2));
        buttons_ye[ic]->setText(QString::number(pos_end.y() * 1e3f, 'f', 2));
        buttons_ze[ic]->setText(QString::number(pos_end.z() * 1e3f, 'f', 2));
    }

    if (update_text_list) {
        updateAutoDimensionTextList();
    }

    ui->obj3dViewer->update();
}

void DimensionCtrl::autoDimensionOnPlane(bool on)
{
    std::vector<MultiDimension*> dimensions;
    std::vector<QCheckBox*>      buttons;
    QItemSelectionModel*         selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            dimensions.emplace_back((MultiDimension*)node->object());
            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_create_auto_dimension->cellWidget(row, AUTO_DIMENSION_LIST_SECTION)
                    ->findChild<QCheckBox*>());
        }
    }

    if (dimensions.size() == 0) {
        return;
    }

    for (auto& dimension : dimensions) {
        dimension->setOnSectionPlane(on);
    }

    for (auto& button : buttons) {
        button->setChecked(on);
    }
}

void DimensionCtrl::removeAutoDimension(bool all)
{
    std::set<Node*> dimensions;

    QItemSelectionModel* selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        if (all || selection_model->isRowSelected(row)) {
            Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                             ->data(Qt::UserRole)
                             .toULongLong();
            dimensions.insert(node);
        }
    }

    if (all) {
        ui->tableWidget_create_auto_dimension->setRowCount(0);
    }
    else {
        for (int row = ui->tableWidget_create_auto_dimension->rowCount() - 1; row >= 0; row--) {
            if (selection_model->isRowSelected(row)) {
                ui->tableWidget_create_auto_dimension->removeRow(row);
            }
        }
    }

    ui->obj3dViewer->sceneView()->sceneGraph()->rootNode()->removeChild(dimensions);
    ui->obj3dViewer->update();

    /// カウントゼロにする
    if (ui->tableWidget_create_auto_dimension->rowCount() == 0) {
        m_auto_dim_count = 0;
        clearAutoDimension();
    }
}

void DimensionCtrl::updateAutoDimensionTextList()
{
    if (m_curent_text_auto_dimension == nullptr) {
        clearAutoDimensionText();
        return;
    }

    MultiDimension* multi_dimension = (MultiDimension*)m_curent_text_auto_dimension->object();

    ui->tableWidget_create_auto_dimension_text->setRowCount(0);    /// 処理遅くないので全クリアにする
    int count = multi_dimension->count();
    for (int ic = 0; ic < count; ++ic) {
        addAutoDimensionText(multi_dimension, ic);
    }

    /// 全クリアなので動作しないがとりあえず残しておく
    int row_count = ui->tableWidget_create_auto_dimension_text->rowCount();
    for (int ic = count; ic < row_count; ++ic) {
        addAutoDimensionText(nullptr, ic);
    }
}

void DimensionCtrl::addAutoDimensionText(MultiDimension* dimension, int index)
{
    auto table_widget = ui->tableWidget_create_auto_dimension_text;

    table_widget->blockSignals(true);

    int row_count = table_widget->rowCount();
    if (index < row_count) {
        int column = 1;
        // QTableWidgetItem* number_item = table_widget->item(index, column++); /// 不変

        if (dimension) {
            QTableWidgetItem* material_item = table_widget->item(index, column++);
            material_item->setText(QString::fromStdWString(dimension->material(index)));

            QTableWidgetItem* value_item = table_widget->item(index, column++);
            value_item->setText(QString::fromStdWString(dimension->valueString(index)));

            QTableWidgetItem* material_org_item = table_widget->item(index, column++);
            material_org_item->setText(QString::fromStdWString(dimension->materialOriginal(index)));
        }
        else {
            QTableWidgetItem* material_item = table_widget->item(index, column++);
            material_item->setText("");

            QTableWidgetItem* value_item = table_widget->item(index, column++);
            value_item->setText("");

            QTableWidgetItem* material_org_item = table_widget->item(index, column++);
            material_org_item->setText("");
        }
    }
    else {
        for (int ic = row_count; ic <= index; ++ic) {
            table_widget->insertRow(ic);
            table_widget->setRowHeight(ic, 13);
            table_widget->setVerticalHeaderItem(ic, new QTableWidgetItem(""));

            int column = 0;

            /// Number
            QTableWidgetItem* number_item = new QTableWidgetItem(QString::number(ic + 1));
            number_item->setFlags(number_item->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
            table_widget->setItem(ic, column++, number_item);

            if (ic == index && dimension) {
                /// Material
                QTableWidgetItem* material_item =
                    new QTableWidgetItem(QString::fromStdWString(dimension->material(index)));
                material_item->setFlags(material_item->flags() | Qt::ItemIsEditable);
                table_widget->setItem(ic, column++, material_item);

                /// Value
                QTableWidgetItem* value_item =
                    new QTableWidgetItem(QString::fromStdWString(dimension->valueString(index)));
                value_item->setFlags(value_item->flags() & ~Qt::ItemIsEditable);
                // value_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                table_widget->setItem(ic, column++, value_item);

                /// Material(original)
                QTableWidgetItem* material_org_item =
                    new QTableWidgetItem(QString::fromStdWString(dimension->materialOriginal(index)));
                material_org_item->setFlags(material_org_item->flags() & ~Qt::ItemIsEditable);
                table_widget->setItem(ic, column++, material_org_item);
            }
            else {
                /// Material
                QTableWidgetItem* material_item = new QTableWidgetItem("");
                material_item->setFlags(material_item->flags() | Qt::ItemIsEditable);
                table_widget->setItem(ic, column++, material_item);

                /// Value
                QTableWidgetItem* value_item = new QTableWidgetItem("");
                value_item->setFlags(value_item->flags() & ~Qt::ItemIsEditable);
                // value_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                table_widget->setItem(ic, column++, value_item);

                /// Material(original)
                QTableWidgetItem* material_org_item = new QTableWidgetItem("");
                material_org_item->setFlags(material_org_item->flags() & ~Qt::ItemIsEditable);
                table_widget->setItem(ic, column++, material_org_item);
            }
        }
    }

    table_widget->blockSignals(false);
    /*
     *     /// Name
    QTableWidgetItem* noItem = new QTableWidgetItem(QString::fromStdWString(dimension->name()));
    noItem->setFlags(noItem->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    noItem->setCheckState(ui->checkBox_create_dimension_disp_name->isChecked() ? Qt::Checked : Qt::Unchecked);
    table_widget->setItem(row, column++, noItem);
    dimension_object->setDispName(ui->checkBox_create_dimension_disp_name->isChecked());

    /// Distance
    QTableWidgetItem* distanceItem = new QTableWidgetItem(QString::fromStdWString(dimension_object->createText(true)));
    distanceItem->setFlags(distanceItem->flags() & ~Qt::ItemIsEditable);    // ReadOnlyに設定
    table_widget->setItem(row, column++, distanceItem);*/
}

QColor DimensionCtrl::adjustColor(const QColor& color, int adjustment)
{
    int r = color.red() + adjustment;
    int g = color.green() + adjustment;
    int b = color.blue() + adjustment;

    // RGBの値を0から255の範囲に制限
    r = qBound(0, r, 255);
    g = qBound(0, g, 255);
    b = qBound(0, b, 255);

    return QColor(r, g, b);
}

void DimensionCtrl::selectDimension(Node* dimension_node)
{
    for (int row = 0; row < ui->tableWidget_create_dimension_list->rowCount(); row++) {
        Node* node = (Node*)ui->tableWidget_create_dimension_list->item(row, DIMENSION_NODE_ITEM)
                         ->data(Qt::UserRole)
                         .toULongLong();
        if (node == dimension_node) {
            auto save_selection_mode = ui->tableWidget_create_dimension_list->selectionMode();
            ui->tableWidget_create_dimension_list->setSelectionMode(QAbstractItemView::SingleSelection);
            ui->tableWidget_create_dimension_list->selectRow(row);
            ui->tableWidget_create_dimension_list->setSelectionMode(save_selection_mode);
            return;
        }
    }

    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                         ->data(Qt::UserRole)
                         .toULongLong();
        if (node == dimension_node) {
            auto save_selection_mode = ui->tableWidget_create_auto_dimension->selectionMode();
            ui->tableWidget_create_auto_dimension->setSelectionMode(QAbstractItemView::SingleSelection);
            ui->tableWidget_create_auto_dimension->selectRow(row);
            ui->tableWidget_create_auto_dimension->setSelectionMode(save_selection_mode);
            return;
        }
    }
}

bool DimensionCtrl::specifyDimensionPoints(SceneView* scene_view, bool auto_dimension, Point3f& point0, Point3f& point1,
                                           Dimension::Type& dimension_type, CreateDimensionType& create_type,
                                           bool temporary, Planef* check_on_olane)
{
    Point3f new_point1 = point1;

    dimension_type = Dimension::Type::XYZDistance;

    create_type = auto_dimension ? createAutoDimensionType() : createDimensionType();
    if (create_type == CreateDimensionType::Min) {
        /// 何もしない
    }
    else if (isXYZDimension(create_type)) {
        if (create_type == CreateDimensionType::X) {
            dimension_type = Dimension::Type::XDistance;
        }
        else if (create_type == CreateDimensionType::Y) {
            dimension_type = Dimension::Type::YDistance;
        }
        else if (create_type == CreateDimensionType::Z) {
            dimension_type = Dimension::Type::ZDistance;
        }
        else if (create_type == CreateDimensionType::XYZ) {
            Point3f point1_x = point1;
            point1_x.setY(point0.y());
            point1_x.setZ(point0.z());

            float length   = (point1_x - point1).length2();
            dimension_type = Dimension::Type::XDistance;
            create_type    = CreateDimensionType::X;

            Point3f point1_y = point1;
            point1_y.setX(point0.x());
            point1_y.setZ(point0.z());
            float temp_length = (point1_y - point1).length2();
            if (length > temp_length) {
                length         = temp_length;
                dimension_type = Dimension::Type::YDistance;
                create_type    = CreateDimensionType::Y;
            }

            Point3f point1_z = point1;
            point1_z.setX(point0.x());
            point1_z.setY(point0.y());
            temp_length = (point1_z - point1).length2();
            if (length > temp_length) {
                length         = temp_length;
                dimension_type = Dimension::Type::ZDistance;
                create_type    = CreateDimensionType::Z;
            }
        }
        else {
            assert(0);
        }

        /// 自動でないときはとりあえず元の座標も保持しているので
        if (auto_dimension) {
            if (new_point1 == point0) {
                new_point1 = point0 + Point3f(1, 1, 1);    /// 適当にずらす
            }
            switch (dimension_type) {
                case Dimension::Type::XDistance:
                    new_point1.setY(point0.y());
                    new_point1.setZ(point0.z());
                    break;
                case Dimension::Type::YDistance:
                    new_point1.setX(point0.x());
                    new_point1.setZ(point0.z());
                    break;
                case Dimension::Type::ZDistance:
                    new_point1.setX(point0.x());
                    new_point1.setY(point0.y());
                    break;
            }
        }
    }
    else {
        if (create_type == CreateDimensionType::Vertical) {    /// 垂直に投影
            Point3f vec = point1 - point0;

            /// 2点目の指定なし
            if (vec.length2() == 0.0f) {
                Planef plane;
                if (!check_on_olane) {
                    plane.setFromNormalAndPoint(scene_view->cameraDir(point0), point0);
                    check_on_olane = &plane;
                }

                /// 暫定: 垂直にずらした点を採用
                float   dist_on_plane = FLT_MAX;
                Point3f temp_point1;

                std::vector<Picking::SIntersectionInfo> intersections;
                Picking                                 picking(scene_view);

                int   move_screen_length = 10;
                float move_length        = scene_view->objectMove(0, move_screen_length).length();
                float move_length_2      = scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 100.0;
                if (move_length > move_length_2) {
                    move_screen_length = (move_length_2 / move_length) * move_screen_length;
                    if (move_screen_length < 1) move_screen_length = 1;
                    move_length = move_length_2;
                }

                /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
                if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                    /// 上にずらす
                    Point3f screen_pt0      = scene_view->projectToScreen(point0);
                    Point3f screen_temp_pt1 = screen_pt0 + Point3f(0, move_screen_length, 0);
                    temp_point1             = scene_view->unprojectFromScreen(screen_temp_pt1);
                }
                else {
                    /// 上にずらす
                    temp_point1 = scene_view->cameraUp() * move_length + point0;
                }
                picking.setOnlyMinimum(true);
                picking.bodyLineIntersection(temp_point1, scene_view->cameraDir(temp_point1), true, intersections,
                                             scene_view->sceneGraph()->lengthUnit().epsilonZero());
                if (intersections.size() > 0) {
                    temp_point1   = scene_view->cameraDir(temp_point1) * intersections[0].m_dist_line_dir + temp_point1;
                    dist_on_plane = check_on_olane->distanceToPoint(temp_point1);
                    point1        = temp_point1;
                }

                /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
                if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                    /// 下にずらす
                    Point3f screen_pt0      = scene_view->projectToScreen(point0);
                    Point3f screen_temp_pt1 = screen_pt0 + Point3f(0, -move_screen_length, 0);
                    temp_point1             = scene_view->unprojectFromScreen(screen_temp_pt1);
                }
                else {
                    /// 下にずらす
                    temp_point1 = -scene_view->cameraUp() * move_length + point0;
                }
                intersections.clear();
                picking.setOnlyMinimum(true);
                picking.bodyLineIntersection(temp_point1, scene_view->cameraDir(temp_point1), true, intersections,
                                             scene_view->sceneGraph()->lengthUnit().epsilonZero());
                if (intersections.size() > 0) {
                    temp_point1 = scene_view->cameraDir(temp_point1) * intersections[0].m_dist_line_dir + temp_point1;
                    if (dist_on_plane > fabs(check_on_olane->distanceToPoint(temp_point1))) {
                        point1 = temp_point1;
                    }
                }

                vec = point1 - point0;
                if (vec.length2() == 0.0f) {
                    return false;
                }

                new_point1 = point1;
            }
            else {
                /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
                if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                    Point3f screen_pt0 = scene_view->projectToScreen(point0);
                    Point3f screen_pt1 = scene_view->projectToScreen(point1);

                    float   dy             = screen_pt1.y() - screen_pt0.y();
                    Point3f screen_new_pt1 = screen_pt0 + Point3f(0, dy, 0);
                    screen_new_pt1.setZ(screen_pt1.z());    /// 一応Z調整

                    new_point1 = scene_view->unprojectFromScreen(screen_new_pt1);
                }
                else {
                    /// ビューの垂直に投影
                    const auto& camera_up = scene_view->cameraUp();
                    Point3f     vec       = point1 - point0;

                    float dir_length = camera_up * vec;
                    new_point1       = point0 + camera_up * dir_length;
                }
            }
        }
        else if (create_type == CreateDimensionType::Horizontal) {    /// 水平に投影
            Point3f vec = point1 - point0;

            /// 2点目の指定なし
            if (vec.length2() == 0.0f) {
                Planef plane;
                if (!check_on_olane) {
                    plane.setFromNormalAndPoint(scene_view->cameraDir(point0), point0);
                    check_on_olane = &plane;
                }

                /// 暫定: 水平にずらした点を採用
                float   dist_on_plane = FLT_MAX;
                Point3f temp_point1;

                std::vector<Picking::SIntersectionInfo> intersections;
                Picking                                 picking(scene_view);

                int   move_screen_length = 10;
                float move_length        = scene_view->objectMove(0, move_screen_length).length();
                float move_length_2      = scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 100.0;
                if (move_length > move_length_2) {
                    move_screen_length = (move_length_2 / move_length) * move_screen_length;
                    if (move_screen_length < 1) move_screen_length = 1;
                    move_length = move_length_2;
                }

                /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
                if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                    /// 右にずらす
                    Point3f screen_pt0      = scene_view->projectToScreen(point0);
                    Point3f screen_temp_pt1 = screen_pt0 + Point3f(move_screen_length, 0, 0);
                    temp_point1             = scene_view->unprojectFromScreen(screen_temp_pt1);
                }
                else {
                    /// 右にずらす
                    temp_point1 = scene_view->cameraRight() * move_length + point0;
                }
                picking.setOnlyMinimum(true);
                picking.bodyLineIntersection(temp_point1, scene_view->cameraDir(temp_point1), true, intersections,
                                             scene_view->sceneGraph()->lengthUnit().epsilonZero());
                if (intersections.size() > 0) {
                    temp_point1   = scene_view->cameraDir(temp_point1) * intersections[0].m_dist_line_dir + temp_point1;
                    dist_on_plane = check_on_olane->distanceToPoint(temp_point1);
                    point1        = temp_point1;
                }

                /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
                if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                    /// 左にずらす
                    Point3f screen_pt0      = scene_view->projectToScreen(point0);
                    Point3f screen_temp_pt1 = screen_pt0 + Point3f(-move_screen_length, 0, 0);
                    temp_point1             = scene_view->unprojectFromScreen(screen_temp_pt1);
                }
                else {
                    /// 左にずらす
                    temp_point1 = -scene_view->cameraRight() * move_length + point0;
                }
                intersections.clear();
                picking.setOnlyMinimum(true);
                picking.bodyLineIntersection(temp_point1, scene_view->cameraDir(temp_point1), true, intersections,
                                             scene_view->sceneGraph()->lengthUnit().epsilonZero());
                if (intersections.size() > 0) {
                    temp_point1 = scene_view->cameraDir(temp_point1) * intersections[0].m_dist_line_dir + temp_point1;
                    if (dist_on_plane > fabs(check_on_olane->distanceToPoint(temp_point1))) {
                        point1 = temp_point1;
                    }
                }

                vec = point1 - point0;
                if (vec.length2() == 0.0f) {
                    return false;
                }

                new_point1 = point1;
            }
            else {
                /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
                if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                    Point3f screen_pt0 = scene_view->projectToScreen(point0);
                    Point3f screen_pt1 = scene_view->projectToScreen(point1);

                    float   dx             = screen_pt1.x() - screen_pt0.x();
                    Point3f screen_new_pt1 = screen_pt0 + Point3f(dx, 0, 0);
                    screen_new_pt1.setZ(screen_pt1.z());    /// 一応Z調整

                    new_point1 = scene_view->unprojectFromScreen(screen_new_pt1);
                }
                else {
                    /// ビューの水平に投影
                    const auto& camera_up         = scene_view->cameraUp();
                    const auto& camera_dir        = scene_view->cameraDir();
                    Point3f     camera_horizontal = (camera_up ^ camera_dir).normalized();

                    float dir_length = camera_horizontal * vec;
                    new_point1       = point0 + camera_horizontal * dir_length;
                }
            }
        }
        else if (create_type == CreateDimensionType::Diagonal) {    /// 対角に投影
            /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
            if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                Point3f screen_pt0 = scene_view->projectToScreen(point0);
                Point3f screen_pt1 = scene_view->projectToScreen(point1);

                float dx = screen_pt1.x() - screen_pt0.x();

                float length = FLT_MAX;
                for (int ic = 0; ic < 4; ++ic) {
                    float flag_x = (ic == 0 || ic == 2) ? 1 : -1;
                    float flag_y = (ic == 0 || ic == 1) ? 1 : -1;

                    Point3f screen_pos = screen_pt0 + Point3f(flag_x * dx, flag_y * dx, 0);
                    screen_pos.setZ(screen_pt1.z());

                    float check_length = (screen_pos - screen_pt1).length2();
                    if (check_length < length) {
                        length     = check_length;
                        new_point1 = scene_view->unprojectFromScreen(screen_pos);
                    }
                }
            }
            else {
                float zero_tol = scene_view->sceneGraph()->lengthUnit().epsilonZero();

                const auto& camera_up         = scene_view->cameraUp().normalized();
                const auto& camera_dir        = scene_view->cameraDir();
                Point3f     camera_horizontal = (camera_up ^ camera_dir).normalized();

                Point3f diagonal_0 = (camera_up + camera_horizontal).normalized();
                Point3f diagonal_1 = (camera_up - camera_horizontal).normalized();

                Point3f temp, pos[4];
                if (closestPoints(point0, diagonal_0, point1, camera_up, pos[0], temp, zero_tol)) {
                    if (closestPoints(point0, diagonal_1, point1, camera_up, pos[2], temp, zero_tol)) {
                        float   length         = FLT_MAX;
                        Point3f new_point_temp = new_point1;
                        for (int ic = 0; ic < 4; ic += 2) {
                            if (length > (pos[ic] - point1).length2()) {
                                length         = (pos[ic] - point1).length2();
                                new_point_temp = pos[ic];
                            }
                        }
                        new_point1 = new_point_temp;
                    }
                }
            }
        }
        else if (create_type == CreateDimensionType::VHD) {    /// 垂直/水平/対角に投影
            /// 分岐しなくてもいい（Orthoも同じ結果になる）がOrthoは簡易処理/高精度なので分けておく
            if (scene_view->projectionType() != SceneView::ProjectionType::Ortho) {
                Point3f screen_pt0 = scene_view->projectToScreen(point0);
                Point3f screen_pt1 = scene_view->projectToScreen(point1);

                float dx = screen_pt1.x() - screen_pt0.x();
                float dy = screen_pt1.y() - screen_pt0.y();

                Point3f screen_pos;
                float   length       = FLT_MAX;
                float   check_length = FLT_MAX;

                /// 垂直
                screen_pos = screen_pt0 + Point3f(0, dy, 0);
                screen_pos.setZ(screen_pt1.z());
                check_length = (screen_pos - screen_pt1).length2();
                length       = check_length;
                new_point1   = scene_view->unprojectFromScreen(screen_pos);
                create_type  = CreateDimensionType::Vertical;

                /// 水平
                screen_pos = screen_pt0 + Point3f(dx, 0, 0);
                screen_pos.setZ(screen_pt1.z());
                check_length = (screen_pos - screen_pt1).length2();
                if (check_length < length) {
                    length      = check_length;
                    new_point1  = scene_view->unprojectFromScreen(screen_pos);
                    create_type = CreateDimensionType::Horizontal;
                }

                /// 対角
                for (int ic = 0; ic < 4; ++ic) {
                    float flag_x = (ic == 0 || ic == 2) ? 1 : -1;
                    float flag_y = (ic == 0 || ic == 1) ? 1 : -1;

                    screen_pos = screen_pt0 + Point3f(flag_x * dx, flag_y * dx, 0);
                    screen_pos.setZ(screen_pt1.z());

                    check_length = (screen_pos - screen_pt1).length2();
                    if (check_length < length) {
                        length      = check_length;
                        new_point1  = scene_view->unprojectFromScreen(screen_pos);
                        create_type = CreateDimensionType::Diagonal;
                    }
                }
            }
            else {
                const auto& camera_up         = scene_view->cameraUp().normalized();
                Point3f     camera_horizontal = (camera_up ^ scene_view->cameraDir()).normalized();

                Point3f vec        = point1 - point0;
                float   ver_length = camera_up * vec;
                Point3f ver_point  = point0 + camera_up * ver_length;

                float   hor_length = camera_horizontal * vec;
                Point3f hor_point  = point0 + camera_horizontal * hor_length;

                float length      = (ver_point - point1).length2();
                float temp_length = (hor_point - point1).length2();
                if (temp_length < length) {
                    length      = temp_length;
                    new_point1  = hor_point;
                    create_type = CreateDimensionType::Horizontal;
                }
                else {
                    new_point1  = ver_point;
                    create_type = CreateDimensionType::Vertical;
                }

                Point3f diagonal_0 = (camera_up + camera_horizontal).normalized();
                Point3f diagonal_1 = (camera_up - camera_horizontal).normalized();

                float zero_tol = scene_view->sceneGraph()->lengthUnit().epsilonZero();

                Point3f temp, pos[4];
                if (closestPoints(point0, diagonal_0, point1, camera_up, pos[0], temp, zero_tol)) {
                    if (closestPoints(point0, diagonal_1, point1, camera_up, pos[2], temp, zero_tol)) {
                        Point3f new_point_temp = new_point1;
                        for (int ic = 0; ic < 4; ic += 2) {
                            if (length > (pos[ic] - point1).length2()) {
                                length         = (pos[ic] - point1).length2();
                                new_point_temp = pos[ic];
                                create_type    = CreateDimensionType::Diagonal;
                            }
                        }
                        new_point1 = new_point_temp;
                    }
                }
            }
        }
        else {
            assert(0);
        }

        if (new_point1 != point1) {
            /// 使わない（位置指定するピックだったが透視投影対応で廃止。下の処理で問題ないと思う（元は遅かったので分けた記憶。高速化したはず）
            if (0) {    // temporary) {
                Point3f out_point;
                if (ui->obj3dViewer->pickVoxelGL(new_point1, out_point)) {
                    new_point1 = out_point;
                }
                else {
                    return false;
                }
            }
            else {
                std::vector<Picking::SIntersectionInfo> intersections;
                Picking                                 picking(scene_view);
                picking.setOnlyMinimum(true);
                picking.bodyLineIntersection(new_point1, scene_view->cameraDir(new_point1), true, intersections,
                                             scene_view->sceneGraph()->lengthUnit().epsilonValidLength());
                if (intersections.size() > 0) {
                    new_point1 = scene_view->cameraDir(new_point1) * intersections[0].m_dist_line_dir + new_point1;
                }
                else {
                    return false;
                }
            }
        }
    }

    point1 = new_point1;

    return true;
}

bool DimensionCtrl::createDimension(SceneView* scene_view, const Point3f& point0, const Point3f& point1, bool temporary,
                                    bool use_temporary)
{
    Point3f         new_point0 = point0;
    Point3f         new_point1 = point1;
    Dimension::Type dimension_type;

    if (use_temporary && m_temporary_dimension != nullptr) {
        Dimension* temporary_dim = (Dimension*)m_temporary_dimension->object();
        new_point0               = temporary_dim->posStart();
        new_point1               = temporary_dim->posEnd();
        dimension_type           = temporary_dim->dimensionType();
    }
    else {
        CreateDimensionType create_type;
        if (!specifyDimensionPoints(scene_view, false, new_point0, new_point1, dimension_type, create_type,
                                    temporary)) {
            return false;
        }
    }

    auto dimension = Dimension::createObject();
    dimension->setDisplayUnit(ui->checkBox_create_dimension_unit_disp->isChecked());
    dimension->set(new_point0, new_point1, scene_view->sceneGraph()->lengthUnit().currentUnit(),
                   LengthUnit::Unit::Nanometer, 0, dimensionStrSize(), dimension_type);

    dimension->setLineWidth((float)ui->spinBox_create_dimension_linewidth->value());

    if (!temporary) {
        dimension->setColor(dimensionColor());

        auto new_node = scene_view->sceneGraph()->rootNode()->addChild();
        new_node->setObject(dimension.ptr());

        addDimension(new_node);
    }
    else {
        QColor temp_color;
        if (m_dimension_color.lightnessF() >= 0.5) {
            temp_color = adjustColor(m_dimension_color, -20);
        }
        else {
            temp_color = adjustColor(m_dimension_color, 50);
        }
        dimension->setColor(Point4f(temp_color.redF(), temp_color.greenF(), temp_color.blueF(), 1.0f));

        dimension->setName(QString("D%1").arg(m_dim_count + 1, 3, 10, QChar('0')).toStdWString());
        if (ui->checkBox_create_dimension_disp_name->isChecked()) {
            dimension->setDispName(true);
        }
        dimension->setEdgeType((Dimension::EdgeType)ui->comboBox_create_dimension_edge_type->currentIndex());

        auto temporary_root = scene_view->temporaryRootNode();

        temporary_root->removeChild(m_temporary_dimension.ptr());

        auto new_node = temporary_root->addChild();
        new_node->setObject(dimension.ptr());

        m_temporary_dimension = new_node;
    }

    return true;
}

bool DimensionCtrl::addAutoDimensionInfo(
    SceneView* scene_view, const Point3f& new_point0, const Point3f& new_point1,
    const std::vector<Clipping::SectionInfo>&                       sections,
    const std::vector<std::pair<ClippingCtrl::PlaneIndex, Planef>>* current_valid_planes, const Point3f* check_vec,
    bool extend_start, bool extend_end, std::vector<MultiDimension::DimensionInfo>& sorted_dimensions_info)
{
    float valid_length    = scene_view->sceneGraph()->lengthUnit().epsilonValidLength();
    float valid_length_sq = scene_view->sceneGraph()->lengthUnit().epsilonValidLengthSquared();

    Point3f line_vec = new_point1 - new_point0;
    Point3f line_dir = line_vec;
    line_dir.normalize();

    float line_param_max = line_vec.length();

    struct DimPointInfo {
        Point3f      m_point_0;
        Point3f      m_point_1;
        float        m_prod_0;
        float        m_prod_1;
        std::wstring m_material;
        std::wstring m_material_original;
        DimPointInfo(const Point3f& point_0, const Point3f& point_1, float prod_0, float prod_1,
                     const std::wstring& material)
            : m_point_0(point_0)
            , m_point_1(point_1)
            , m_prod_0(prod_0)
            , m_prod_1(prod_1)
            , m_material(material)
            , m_material_original(material)
        {
        }

        void removeSuffix(const std::wstring& suffix)
        {
            if (m_material.size() >= suffix.size()
                && m_material.compare(m_material.size() - suffix.size(), suffix.size(), suffix) == 0) {
                m_material.erase(m_material.size() - suffix.size());
                m_material_original = m_material;    /// Originalからも消す
            }
        }

        void replace(const std::map<std::wstring, std::wstring>& map_replace)
        {
            auto itr_find = map_replace.find(m_material);
            if (itr_find != map_replace.end()) {
                m_material = itr_find->second;
            }
        }
    };
    std::vector<DimPointInfo*> dimension_point_info;

    bool along_plane = (current_valid_planes && check_vec) ? true : false;

    if (!along_plane) {
        for (auto& sec : sections) {
            std::vector<std::tuple<float, float, Point3f, Point3f>> valid_points;
            for (auto& line : sec.m_points) {
                Point3f line_point0 = line.first;
                Point3f line_point1 = line.second;

                float cross0 = (line_dir ^ (line_point0 - new_point0)).length2();
                if (cross0 > valid_length_sq * 100) {
                    continue;
                }

                float cross1 = (line_dir ^ (line_point1 - new_point0)).length2();
                if (cross1 > valid_length_sq * 100) {
                    continue;
                }

                float prod0 = line_dir * (line_point0 - new_point0);
                float prod1 = line_dir * (line_point1 - new_point0);

                if (!extend_start) {
                    if (prod0 < 0.0f) {
                        prod0       = 0.0f;
                        line_point0 = new_point0;
                    }
                    if (prod1 < 0.0f) {
                        prod1       = 0.0f;
                        line_point1 = new_point0;
                    }
                }

                if (!extend_end) {
                    if (prod0 > line_param_max) {
                        prod0       = line_param_max;
                        line_point0 = new_point0 + line_vec;
                    }
                    if (prod1 > line_param_max) {
                        prod1       = line_param_max;
                        line_point1 = new_point0 + line_vec;
                    }
                }

                if (prod0 == 0.0f && prod1 == 0.0f) {
                    continue;
                }
                if (prod0 == line_param_max && prod1 == line_param_max) {
                    continue;
                }

                /// 入れ替え
                if (prod1 < prod0) {
                    valid_points.emplace_back(prod1, prod0, line_point1, line_point0);
                }
                else {
                    valid_points.emplace_back(prod0, prod1, line_point0, line_point1);
                }
            }
            std::sort(valid_points.begin(), valid_points.end(),
                      [](const std::tuple<float, float, Point3f, Point3f>& a,
                         const std::tuple<float, float, Point3f, Point3f>& b) {
                          if (std::get<0>(a) < std::get<0>(b))
                              return true;
                          else if (std::get<0>(a) > std::get<0>(b))
                              return false;
                          if (std::get<1>(a) < std::get<1>(b)) return true;
                          // else if (std::get<1>(a) > std::get<1>(b))
                          //     return false;
                          return false;
                      });

            for (int ic = 0; ic < valid_points.size(); ++ic) {
                float    prod0       = std::get<0>(valid_points[ic]);
                float    prod1       = std::get<1>(valid_points[ic]);
                Point3f& line_point0 = std::get<2>(valid_points[ic]);
                Point3f  line_point1 = std::get<3>(valid_points[ic]);

                int jc = ic + 1;
                while (jc < valid_points.size()) {
                    float prod2_0 = std::get<0>(valid_points[jc]);
                    float prod2_1 = std::get<1>(valid_points[jc]);
                    if (prod2_0 - valid_length <= prod1) {
                        if (prod1 < prod2_1) {
                            line_point1 = std::get<3>(valid_points[jc]);
                            prod1       = prod2_1;
                        }
                    }
                    else {
                        break;
                    }
                    ++jc;
                }
                ic = jc - 1;

                if ((line_point0 - line_point1).length2() <= valid_length_sq) {
                    continue;
                }
                dimension_point_info.emplace_back(
                    new DimPointInfo(line_point0, line_point1, prod0, prod1, sec.m_node->name()));
            }
        }
    }
    else {
        const auto& current_planes = *current_valid_planes;

        std::set<int> new_point0_plane;
        for (int ic = 0; ic < current_planes.size(); ++ic) {
            int         index         = current_planes[ic].first;
            const auto& current_plane = current_planes[ic].second;
            if (fabs(current_plane.distanceToPoint(new_point0)) <= valid_length) {
                new_point0_plane.insert(index);
            }
        }
        std::set<int> new_point1_plane;
        for (int ic = 0; ic < current_planes.size(); ++ic) {
            int         index         = current_planes[ic].first;
            const auto& current_plane = current_planes[ic].second;
            if (fabs(current_plane.distanceToPoint(new_point1)) <= valid_length) {
                new_point1_plane.insert(index);
            }
        }

        for (auto& sec : sections) {
            std::vector<std::tuple<float, float, Point3f, Point3f, int>> valid_points;
            for (auto& line : sec.m_points) {
                Point3f line_point0 = line.first;
                Point3f line_point1 = line.second;

                int on_valid_plane = -1;
                for (int ic = 0; ic < current_planes.size(); ++ic) {
                    int         index         = current_planes[ic].first;
                    const auto& current_plane = current_planes[ic].second;
                    if (fabs(current_plane.distanceToPoint(line_point0)) <= valid_length
                        && fabs(current_plane.distanceToPoint(line_point1)) <= valid_length) {
                        on_valid_plane = index;
                        break;
                    }
                }
                if (on_valid_plane < 0) {
                    continue;
                }
                /// 両方線の反対側なら除外
                float check_0 = *check_vec * (line_point0 - new_point0);
                float check_1 = *check_vec * (line_point1 - new_point0);

                if ((check_0 > -valid_length && check_1 >= valid_length)
                    || (check_0 >= valid_length && check_1 > -valid_length)) {
                    bool skip = true;
                    if ((extend_start && new_point0_plane.size() == 1 && new_point0_plane.count(on_valid_plane))
                        || (extend_end && new_point1_plane.size() == 1 && new_point1_plane.count(on_valid_plane))) {
                        skip = false;
                    }
                    if (skip) {
                        continue;
                    }
                }

                float prod0 = line_dir * (line_point0 - new_point0);
                float prod1 = line_dir * (line_point1 - new_point0);

                float org_prod0  = prod0;
                float prod_range = prod1 - prod0;
                if (prod_range == 0.0f) {
                    continue;
                }

                Point3f new_line_point0 = line_point0;
                Point3f new_line_point1 = line_point1;

                if (!extend_start) {
                    if (prod0 < 0.0f) {
                        prod0           = 0.0f;
                        new_line_point0 = (line_point1 - line_point0) * ((0.0f - org_prod0) / prod_range) + line_point0;
                    }
                    if (prod1 < 0.0f) {
                        prod1           = 0.0f;
                        new_line_point1 = (line_point1 - line_point0) * ((0.0f - org_prod0) / prod_range) + line_point0;
                    }
                }

                if (!extend_end) {
                    if (prod0 > line_param_max) {
                        prod0 = line_param_max;
                        new_line_point0 =
                            (line_point1 - line_point0) * ((line_param_max - org_prod0) / prod_range) + line_point0;
                    }
                    if (prod1 > line_param_max) {
                        prod1 = line_param_max;
                        new_line_point1 =
                            (line_point1 - line_point0) * ((line_param_max - org_prod0) / prod_range) + line_point0;
                    }
                }

                if (prod0 == 0.0f && prod1 == 0.0f) {
                    continue;
                }
                if (prod0 == line_param_max && prod1 == line_param_max) {
                    continue;
                }

                line_point1 = new_line_point1;
                line_point0 = new_line_point0;

                /// 入れ替え
                if (prod1 < prod0) {
                    valid_points.emplace_back(prod1, prod0, line_point1, line_point0, on_valid_plane);
                }
                else {
                    valid_points.emplace_back(prod0, prod1, line_point0, line_point1, on_valid_plane);
                }
            }

            std::sort(valid_points.begin(), valid_points.end(),
                      [](const std::tuple<float, float, Point3f, Point3f, int>& a,
                         const std::tuple<float, float, Point3f, Point3f, int>& b) {
                          if (std::get<0>(a) < std::get<0>(b))
                              return true;
                          else if (std::get<0>(a) > std::get<0>(b))
                              return false;
                          if (std::get<1>(a) < std::get<1>(b)) return true;
                          // else if (std::get<1>(a) > std::get<1>(b))
                          //     return false;
                          return false;
                      });

            for (int ic = 0; ic < valid_points.size(); ++ic) {
                float    prod0       = std::get<0>(valid_points[ic]);
                float    prod1       = std::get<1>(valid_points[ic]);
                Point3f& line_point0 = std::get<2>(valid_points[ic]);
                Point3f  line_point1 = std::get<3>(valid_points[ic]);
                int      plane       = std::get<4>(valid_points[ic]);

                int jc = ic + 1;
                while (jc < valid_points.size()) {
                    float prod2_0 = std::get<0>(valid_points[jc]);
                    float prod2_1 = std::get<1>(valid_points[jc]);
                    int   plane2  = std::get<4>(valid_points[jc]);
                    if (plane != plane2) {
                        break;
                    }
                    if (prod2_0 - valid_length <= prod1) {
                        if (prod1 < prod2_1) {
                            line_point1 = std::get<3>(valid_points[jc]);
                            prod1       = prod2_1;
                        }
                    }
                    else {
                        break;
                    }
                    ++jc;
                }
                ic = jc - 1;

                if ((line_point0 - line_point1).length2() <= valid_length_sq) {
                    continue;
                }
                dimension_point_info.emplace_back(
                    new DimPointInfo(line_point0, line_point1, prod0, prod1, sec.m_node->name()));
            }
        }
    }

    std::sort(dimension_point_info.begin(), dimension_point_info.end(),
              [](const DimPointInfo* a, const DimPointInfo* b) {
                  if (a->m_prod_0 < b->m_prod_0)
                      return true;
                  else if (a->m_prod_0 > b->m_prod_0)
                      return false;
                  else if (a->m_prod_1 < b->m_prod_1)
                      return true;
                  return false;
              });

    /// 挿入があるのでリスト構造で扱う
    std::list<DimPointInfo*> dimension_point_info_list(dimension_point_info.begin(), dimension_point_info.end());

    for (auto it = dimension_point_info_list.begin(); it != dimension_point_info_list.end(); ++it) {
        if (*it == nullptr) {
            continue;
        }
        auto dim_point_0 = *it;

        auto jt = std::next(it);
        while (jt != dimension_point_info_list.end()) {
            if (*jt == nullptr) {
                ++jt;
                continue;
            }
            auto dim_point_1 = *jt;

            /// 重なりがない
            if (dim_point_0->m_prod_1 <= dim_point_1->m_prod_0) {
                break;
            }

            /// 完全に重なる
            if (dim_point_0->m_prod_0 == dim_point_1->m_prod_0 && dim_point_0->m_prod_1 == dim_point_1->m_prod_1) {
                delete *jt;
                *jt = nullptr;
            }
            /// 内包する
            else if (dim_point_0->m_prod_1 > dim_point_1->m_prod_1) {
                auto new_dim_point =
                    new DimPointInfo(dim_point_1->m_point_1, dim_point_0->m_point_1, dim_point_1->m_prod_1,
                                     dim_point_0->m_prod_1, dim_point_0->m_material);

                /// ソート関係維持して追加（it/jtより必ず後ろのはずなので、そのままループ回していい）
                /// 暫定: 遅い場合があるかもしれないが、重なり数に依存するのでそこまでではないはず
                auto insert_pos = std::next(jt);
                while (insert_pos != dimension_point_info_list.end()) {
                    if (*insert_pos != nullptr && (*insert_pos)->m_prod_0 > new_dim_point->m_prod_0) {
                        break;
                    }
                    ++insert_pos;
                }
                dimension_point_info_list.insert(insert_pos, new_dim_point);

                dim_point_0->m_point_1 = dim_point_1->m_point_0;
                dim_point_0->m_prod_1  = dim_point_1->m_prod_0;
            }
            /// 内包しないが重なりがある
            else if (dim_point_0->m_prod_1 <= dim_point_1->m_prod_1) {
                /// 始点が同じ
                if (dim_point_0->m_prod_0 == dim_point_1->m_prod_0) {
                    auto new_dim_point =
                        new DimPointInfo(dim_point_0->m_point_1, dim_point_1->m_point_1, dim_point_0->m_prod_1,
                                         dim_point_1->m_prod_1, dim_point_1->m_material);

                    /// 短い方を残す（it残す）
                    /// 一旦jtは消して、新規要素追加する
                    delete *jt;
                    *jt = nullptr;

                    /// ソート関係維持して追加（it/jtより必ず後ろのはずなので、そのままループ回していい）
                    /// 暫定: 遅い場合があるかもしれないが、重なり数に依存するのでそこまでではないはず
                    auto insert_pos = std::next(jt);
                    while (insert_pos != dimension_point_info_list.end()) {
                        if (*insert_pos != nullptr && (*insert_pos)->m_prod_0 > new_dim_point->m_prod_0) {
                            break;
                        }
                        ++insert_pos;
                    }
                    dimension_point_info_list.insert(insert_pos, new_dim_point);
                }
                /// 始点が違う
                else {
                    /// そのまま縮める
                    dim_point_0->m_point_1 = dim_point_1->m_point_0;
                    dim_point_0->m_prod_1  = dim_point_1->m_prod_0;
                }
            }

            ++jt;
        }
    }

    bool ret = false;

    for (auto it = dimension_point_info_list.begin(); it != dimension_point_info_list.end(); ++it) {
        if (*it == nullptr) {
            continue;
        }
        auto dim = *it;

        if ((dim->m_point_0 - dim->m_point_1).length2() <= valid_length_sq) {
            delete *it;
            continue;
        }

        auto dimension_obj = Dimension::createObject();
        dimension_obj->set(dim->m_point_0, dim->m_point_1, scene_view->sceneGraph()->lengthUnit().currentUnit(),
                           LengthUnit::Unit::Nanometer, 0, autoDimensionStrSize(), Dimension::XYZDistance);

        dim->removeSuffix(L"[NoImp:0]");
        dim->replace(m_auto_dimension_material_change);

        sorted_dimensions_info.emplace_back(dimension_obj, dim->m_material, dim->m_material_original);
        ret = true;
        delete *it;
    }
    dimension_point_info_list.clear();
    dimension_point_info.clear();

    return ret;
}

std::vector<Node*> DimensionCtrl::autoDimensions()
{
    std::vector<Node*> dimensions;

    QItemSelectionModel* selection_model = ui->tableWidget_create_auto_dimension->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension->rowCount(); row++) {
        Node* node = (Node*)ui->tableWidget_create_auto_dimension->item(row, AUTO_DIMENSION_NODE_ITEM)
                         ->data(Qt::UserRole)
                         .toULongLong();
        dimensions.emplace_back(node);
    }

    return dimensions;
}

bool DimensionCtrl::createAutoDimension(SceneView* scene_view, const std::vector<Point3f>& points, bool temporary,
                                        bool continuous_end, bool force_along_plane, bool use_temporary)
{
    auto    create_dim_type = createAutoDimensionType();
    Point3f new_point0      = points.size() > 0 ? points[0] : Point3f();
    Point3f new_point1      = points.size() > 1 ? points[1] : new_point0;

    /// プレビューと確定でずれが出るので、確定時は直前のプレビュー情報使う
    bool specified_point = false;
    if (use_temporary && m_temporary_auto_dimension != nullptr) {
        MultiDimension* multi_dim   = (MultiDimension*)m_temporary_auto_dimension->object();
        const auto&     base_points = multi_dim->basePoints();
        if (base_points.size() > 1) {
            new_point0      = base_points[0];
            new_point1      = base_points[1];
            specified_point = true;
        }
    }

    /// 共通変数
    float valid_length = scene_view->sceneGraph()->lengthUnit().epsilonValidLength();

    const auto& camera_dir   = scene_view->cameraDir(new_point1);
    const auto& camera_up    = scene_view->cameraUp(new_point1);
    const auto& camera_right = (camera_up ^ camera_dir).normalized();    // scene_view->cameraRight();

    /// 1点目の通る平面を特定
    ///  ※ VHDで延長のとき、この平面があれば２点目が決まるので先に実施

    std::vector<MultiDimension::OnPlaneInfo> point0_on_planes;

    BoundingBox3f bbox = scene_view->sceneGraph()->rootNode()->shapeDisplayBoundingBox();

    int         point_on_plane_index      = -1;
    int         point_on_plane_list_index = -1;
    Planef      point_on_plane;
    auto        clipping_ctrl  = m_3DForm->clippingCtrl();
    const auto& current_planes = clipping_ctrl->currentValidPlanes(bbox);
    for (int ic = 0; ic < current_planes.size(); ++ic) {
        int         index         = current_planes[ic].first;
        const auto& current_plane = current_planes[ic].second;

        float dist_on_plane = fabs(current_plane.distanceToPoint(new_point0));
        if (dist_on_plane <= valid_length) {
            if (point_on_plane_index < 0) {
                point_on_plane_index      = index;
                point_on_plane_list_index = ic;
                point0_on_planes.emplace_back(index, current_plane);
            }
            else {
                const auto& norm0 = current_planes[point_on_plane_index].second.normal();
                const auto& norm1 = current_plane.normal();
                if (fabs(norm0 * camera_dir) < fabs(norm1 * camera_dir)) {
                    point_on_plane_index      = index;
                    point_on_plane_list_index = ic;
                }

                point0_on_planes.emplace_back(index, current_plane);
            }
        }
    }
    if (point_on_plane_list_index >= 0) {
        point_on_plane = current_planes[point_on_plane_list_index].second;
    }

    if (!continuous_end) {
        /// プレビュー点を使うので何もしない
        if (specified_point) {
        }
        /// 2点目を特定
        else {
            Dimension::Type dimension_type;
            if (!specifyDimensionPoints(scene_view, true, new_point0, new_point1, dimension_type, create_dim_type,
                                        temporary, point_on_plane_index >= 0 ? &point_on_plane : nullptr)) {
                return false;
            }
        }
    }

    bool extend_start = ui->checkBox_create_auto_dimension_extend->isChecked();
    bool extend_end   = ui->checkBox_create_auto_dimension_extend->isChecked();

    /// 連続の場合の判定
    if (extend_start) {
        if (isContinuousAutoDimension()) {
            if (temporary) {
                if (isDuringContinuousAutoDimension()) {
                    extend_start = false;
                    extend_end   = true;
                }
                else {
                    extend_start = true;
                    extend_end   = true;
                }
            }
            else if (!isDuringContinuousAutoDimension()) {
                extend_start = true;
                extend_end   = false;
            }
            else if (continuous_end) {
                extend_start = false;
                extend_end   = true;
            }
            else {
                extend_start = false;
                extend_end   = false;
            }
        }
    }

    if (temporary) {
        auto dimension = MultiDimension::createObject();
        dimension->setTemporary(true);
        dimension->setExtendStart(extend_start);
        dimension->setExtendEnd(extend_end);

        if (ui->checkBox_create_auto_dimension_along_plane->isEnabled()
            && ui->checkBox_create_auto_dimension_along_plane->isChecked()) {
            dimension->setOnSectionPlane(true);
        }
        else {
            dimension->setOnSectionPlane(false);
        }

        QColor temp_color;
        if (m_auto_dimension_color.lightnessF() >= 0.5) {
            temp_color = adjustColor(m_auto_dimension_color, -20);
        }
        else {
            temp_color = adjustColor(m_auto_dimension_color, 50);
        }
        dimension->setColor(temp_color.redF(), temp_color.greenF(), temp_color.blueF());
        dimension->setBasePoints({new_point0, new_point1});
        dimension->setLineWidth((float)ui->spinBox_create_auto_dimension_linewidth->value());

        auto temporary_root = scene_view->temporaryRootNode();

        temporary_root->removeChild(m_temporary_auto_dimension.ptr());

        auto new_node = temporary_root->addChild();
        new_node->setObject(dimension.ptr());

        m_temporary_auto_dimension = new_node;

        return true;
    }

    /// 単純なメッシュとの３次元交点が内外判定が微妙なので断面から求める

    /// Line方向
    Point3f line_dir = new_point1 - new_point0;
    line_dir.normalize();
    if (line_dir.length2() == 0.0) {
        return false;
    }

    /// Lineが乗る平面と、それに直交して断面を出す平面
    Planef clip_plane;
    Planef sect_plane;

    /// 平面上判定
    Point3f axis  = camera_dir;
    Point3f axis2 = line_dir ^ axis;
    if (axis2.length2() == 0.0f) {
        axis  = camera_right;
        axis2 = line_dir ^ axis;
    }
    axis = axis2 ^ line_dir;

    clip_plane = Planef(axis, new_point0);
    sect_plane = Planef(axis2, new_point0);

    bool along_plane = force_along_plane;
    /// 平面を補正
    if (point_on_plane_index >= 0) {
        bool on_plane_1 = false;
        for (auto& plane : point0_on_planes) {
            if (fabs(plane.m_plane.distanceToPoint(new_point1)) <= valid_length) {
                on_plane_1 = true;
                break;
            }
        }

        /// 2点目も同一平面に乗っている場合
        if (on_plane_1) {
            clip_plane = Planef(Point3f(0, 0, 0), new_point0);
        }
        else {
            if (ui->checkBox_create_auto_dimension_along_plane->isEnabled()
                && ui->checkBox_create_auto_dimension_along_plane->isChecked()) {
                /// 断面沿いがONの場合
                along_plane = true;
                /// 面沿いにする
                clip_plane = Planef(Point3f(0, 0, 0), new_point0);
            }
        }
    }

    /// 必要ならクリッピングしたのち、その形状に対してさらに断面とって断面線取得
    std::vector<Clipping::SectionInfo> sections;

    Clipping clipping(scene_view->sceneGraph());
    clipping.sectionLineForAutoLength(clip_plane, sect_plane, true, sections, nullptr);

    std::vector<MultiDimension::DimensionInfo> sorted_dimensions_info;
    if (!addAutoDimensionInfo(scene_view, new_point0, new_point1, sections, along_plane ? &current_planes : nullptr,
                              along_plane ? &axis : nullptr, extend_start, extend_end, sorted_dimensions_info)) {
        return false;
    }

    /// 連続でない場合
    bool reverse = false;
    if (points.size() == 1 || !isContinuousAutoDimension()) {
        /// 画面投影して判断
        auto screen_line_pt0 = scene_view->projectToScreen(new_point0);
        auto screen_line_pt1 = scene_view->projectToScreen(new_point1);
        auto vector          = screen_line_pt1 - screen_line_pt0;

        if (fabs(vector.x()) > 1.0e-8f || fabs(vector.y()) > 1.0e-8f) {
            float angle         = atan2(-vector.y(), vector.x());    /// ラジアン
            float angle_degrees = angle * (180.0f / M_PI);           /// わかりやすく度に変換

            // DEBUG() << "ANGLE " << angle_degrees;

            float angle_adj_y = 15.0f;

            if (angle_degrees >= 45.0f - angle_adj_y && angle_degrees <= 135.0f + angle_adj_y) {
                /// Y軸付近（上方向）
                reverse = true;
            }
            else if (angle_degrees >= -135.0f - angle_adj_y && angle_degrees <= -45.0f + angle_adj_y) {
                /// Y軸付近（下方向）
            }
            else if (angle_degrees >= -45.0f && angle_degrees <= 45.0f) {
                /// X軸付近（右方向）
            }
            else if (angle_degrees >= 135.0f || angle_degrees <= -135.0f) {
                /// X軸付近（左方向）
                reverse = true;
            }
        }
    }

    if (isContinuousAutoDimension()) {
        bool first = false;
        if (!isDuringContinuousAutoDimension()) {
            m_continuous_auto_dimension = scene_view->sceneGraph()->rootNode()->addChild();
            m_continuous_auto_dimension->setObject(MultiDimension::createObject().ptr());
            first = true;
        }

        MultiDimension* dimension = (MultiDimension*)m_continuous_auto_dimension->object();

        dimension->addDimensions(sorted_dimensions_info);

        dimension->createBasePoints(valid_length);

        std::vector<MultiDimension::OnPlaneInfo> on_planes;
        for (auto& current_plane : current_planes) {
            on_planes.emplace_back((int)current_plane.first, current_plane.second);
        }
        dimension->createBasePointPlanes(on_planes, valid_length * 10);

        if (first) {
            dimension->setTextAlign(autoDimensionTextAlign());
            dimension->setDisplayValue(ui->checkBox_create_auto_dimension_disp_value->isChecked());
            dimension->setDisplayMaterial(ui->checkBox_create_auto_dimension_disp_material->isChecked());
            dimension->setLineWidth((float)ui->spinBox_create_auto_dimension_linewidth->value());
            dimension->setDisplayUnit(ui->checkBox_create_auto_dimension_unit_disp->isChecked());
            dimension->setOnSectionPlane(ui->checkBox_create_auto_dimension_section->isChecked());
            dimension->setColor(autoDimensionColor());

            dimension->updateText();

            addAutoDimension(m_continuous_auto_dimension.ptr(), -1, first);
        }
        else {
            /// 自身の設定を全体に再設定
            dimension->setTextAlign(dimension->textAlign());
            dimension->setDisplayValue(dimension->isDisplayValue());
            dimension->setDisplayMaterial(dimension->isDisplayMaterial());
            dimension->setLineWidth(dimension->lineWidth());
            dimension->setDisplayUnit(dimension->isDisplayUnit());
            dimension->setOnSectionPlane(dimension->isOnSectionPlane());
            dimension->setColor(dimension->color());
            dimension->setDispName(dimension->dispName());
            dimension->setName(dimension->name());
            dimension->setEdgeType(dimension->edgeType());

            dimension->updateText();
            updateAutoDimension(m_continuous_auto_dimension.ptr());
        }

        // int remove_row = removeAutoDimensionList(m_continuous_auto_dimension.ptr());
        // addAutoDimension(m_continuous_auto_dimension.ptr(), remove_row, first);
        // if (m_auto_dim_count == 0) {    /// !firstで連続のときremoveAutoDimensionListで0になったままなので
        //     m_auto_dim_count = 1;
        // }
    }
    else {
        auto dimension = MultiDimension::createObject();

        dimension->addDimensions(sorted_dimensions_info);

        if (reverse) {
            dimension->reverse();
        }

        auto new_node = scene_view->sceneGraph()->rootNode()->addChild();
        new_node->setObject(dimension.ptr());

        dimension->createBasePoints(valid_length);
        std::vector<MultiDimension::OnPlaneInfo> on_planes;
        for (auto& current_plane : current_planes) {
            on_planes.emplace_back((int)current_plane.first, current_plane.second);
        }
        dimension->createBasePointPlanes(on_planes, valid_length * 10);

        dimension->setTextAlign(autoDimensionTextAlign());
        dimension->setDisplayValue(ui->checkBox_create_auto_dimension_disp_value->isChecked());
        dimension->setDisplayMaterial(ui->checkBox_create_auto_dimension_disp_material->isChecked());
        dimension->setOnSectionPlane(ui->checkBox_create_auto_dimension_section->isChecked());
        dimension->setLineWidth((float)ui->spinBox_create_auto_dimension_linewidth->value());
        dimension->setDisplayUnit(ui->checkBox_create_auto_dimension_unit_disp->isChecked());
        dimension->setColor(autoDimensionColor());
        dimension->updateText();

        addAutoDimension(new_node);
    }

    return true;
}

bool DimensionCtrl::updateAutoDimensionOnSection()
{
    auto dimensions = autoDimensions();
    if (dimensions.size() == 0) {
        return false;
    }

    BoundingBox3f bbox           = ui->obj3dViewer->sceneView()->sceneGraph()->rootNode()->shapeDisplayBoundingBox();
    auto          clipping_ctrl  = m_3DForm->clippingCtrl();
    const auto&   current_planes = clipping_ctrl->currentValidPlanes(bbox, true);

    std::vector<Planef> current_valid_planes_list;

    for (auto& dimension : dimensions) {
        // if (!dimension.isAlive()) {
        //     continue;
        // }
        MultiDimension* dim_obj = (MultiDimension*)dimension->object();
        if (!dim_obj->isOnSectionPlane()) {
            continue;
        }

        /// 初回
        if (current_valid_planes_list.size() == 0) {
            current_valid_planes_list.resize(ClippingCtrl::NumOfPlane, Planef());
            for (auto& [index, current_plane] : current_planes) {
                current_valid_planes_list[(int)index] = current_plane;
            }
        }

        const auto& base_points       = dim_obj->basePoints();
        const auto& base_point_planes = dim_obj->basePointPlanes();

        VecPoint3f new_points;
        for (int ic = 1; ic < base_points.size(); ++ic) {
            auto start = base_points[ic - 1];
            auto end   = base_points[ic];

            auto org_line_dir = (end - start).normalized();

            const auto& start_planes = base_point_planes[ic - 1];
            const auto& end_planes   = base_point_planes[ic];

            /// 線の平面
            std::set<int> line_planes;
            for (const auto& start_plane : start_planes) {
                if (end_planes.count(start_plane)) {
                    line_planes.insert(start_plane);
                }
            }

            /// 線の平面に投影
            for (const auto& line_plane : line_planes) {
                const Planef& plane = current_valid_planes_list[line_plane];
                if (plane.normal() == Point3f(0.0f, 0.0f, 0.0f)) {
                    continue;
                }

                start = plane.projectPoint(start);
                end   = plane.projectPoint(end);
            }

            /// 始点を延長or縮小
            for (const auto& start_plane : start_planes) {
                if (line_planes.count(start_plane)) {
                    continue;
                }
                const Planef& plane = current_valid_planes_list[start_plane];
                if (plane.normal() == Point3f(0.0f, 0.0f, 0.0f)) {
                    continue;
                }

                /// 線分と平面の交点で置き換え
                Point3f intersection;
                if (plane.intersectSegment(start, end, intersection, true)) {
                    start = intersection;
                }
            }

            /// 終点を延長or縮小
            for (const auto& end_plane : end_planes) {
                if (line_planes.count(end_plane)) {
                    continue;
                }
                const Planef& plane = current_valid_planes_list[end_plane];
                if (plane.normal() == Point3f(0.0f, 0.0f, 0.0f)) {
                    continue;
                }

                /// 線分と平面の交点で置き換え
                Point3f intersection;
                if (plane.intersectSegment(start, end, intersection, true)) {
                    end = intersection;
                }
            }

            /// 終点が反転している場合
            if (org_line_dir * (end - start) < 0.0f) {
                continue;
            }

            new_points.emplace_back(start);
            new_points.emplace_back(end);
        }

        updateAutoDimensionOnSection(ui->obj3dViewer->sceneView(), bbox, dimension, new_points, false);
    }

    updateAutoDimensions();

    return true;
}

void DimensionCtrl::updateAutoDimensionMaterialNameChange()
{
    auto current_node = m_continuous_auto_dimension;

    auto dimensions = autoDimensions();

    for (auto& dimension_node : dimensions) {
        auto multi_dim = dimension_node->object<MultiDimension>();
        if (multi_dim) {
            multi_dim->setMaterial(m_auto_dimension_material_change);
        }

        if (current_node == dimension_node) {
            current_node = nullptr;
        }
    }
    if (current_node != nullptr) {
        auto multi_dim = current_node->object<MultiDimension>();
        if (multi_dim) {
            multi_dim->setMaterial(m_auto_dimension_material_change);
        }
    }

    updateAutoDimensionTextList();
    ui->obj3dViewer->update();
}

void DimensionCtrl::clearAutoDimensionMaterialNameChange(bool show_msg)
{
    if (m_auto_dimension_material_change.size() > 0) {
        if (show_msg) {
            if (QMessageBox::question(m_3DForm, m_3DForm->windowTitle(), "Material変更をすべて初期化します。")
                != QMessageBox::Yes) {
                return;
            }
        }
        m_auto_dimension_material_change.clear();
        updateAutoDimensionMaterialNameChange();
    }
}

void DimensionCtrl::autoDimensionTextNameChange()
{
    if (!m_curent_text_auto_dimension.valid()) {
        return;
    }

    MultiDimension* multi_dim = m_curent_text_auto_dimension->object<MultiDimension>();

    std::map<std::wstring, std::wstring> map_material_change;
    QItemSelectionModel*                 selection_model = ui->tableWidget_create_auto_dimension_text->selectionModel();
    for (int row = 0; row < ui->tableWidget_create_auto_dimension_text->rowCount(); row++) {
        QModelIndex index = ui->tableWidget_create_auto_dimension_text->model()->index(row, 1);
        if (selection_model->isRowSelected(row) || selection_model->isSelected(index)) {
            map_material_change[multi_dim->materialOriginal(row)] = multi_dim->material(row);
        }
    }

    if (map_material_change.size() == 0) {
        return;
    }

    LineEditDialog dlg("Change Material", (QWidget*)this->parent());

    std::wstring init_name = map_material_change.begin()->second;
    for (auto itr = map_material_change.begin(); itr != map_material_change.end(); ++itr) {
        if (init_name != itr->second) {
            init_name = L"";
            break;
        }
    }
    dlg.m_edit->setText(QString::fromStdWString(init_name));

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const auto& wtext = dlg.m_edit->text().toStdWString();

    for (auto itr = map_material_change.begin(); itr != map_material_change.end(); ++itr) {
        m_auto_dimension_material_change[itr->first] = wtext;
    }

    updateAutoDimensionMaterialNameChange();
}

void DimensionCtrl::autoDimensionTextAllSelect()
{
    ui->tableWidget_create_auto_dimension_text->selectAll();
}
void DimensionCtrl::autoDimensionTextCopy()
{
    ui->tableWidget_create_auto_dimension_text->copy(false);
}
void DimensionCtrl::autoDimensionTextAllCopy()
{
    ui->tableWidget_create_auto_dimension_text->copy(true);
}

void DimensionCtrl::autoDimensionTextAllCopyExceptOrg()
{
    std::map<int, std::set<int>> mapSelectIndexes;

    for (int ic = 0; ic < ui->tableWidget_create_auto_dimension_text->rowCount(); ++ic) {
        auto& set_column = mapSelectIndexes[ic];
        for (int jc = 0; jc < ui->tableWidget_create_auto_dimension_text->columnCount() - 1; ++jc) {
            set_column.insert(jc);
        }
    }
    ui->tableWidget_create_auto_dimension_text->copy(mapSelectIndexes);
}

void DimensionCtrl::showDimension(bool show)
{
    dimensionShow(show, true);
}

void DimensionCtrl::showAutoDimension(bool show)
{
    autoDimensionShow(show, true);
}

bool DimensionCtrl::updateAutoDimensionOnSection(SceneView* scene_view, const BoundingBox3f& bbox,
                                                 const Point3f& new_point0, const Point3f& new_point1,
                                                 std::vector<MultiDimension::DimensionInfo>& sorted_dimensions_info)
{
    /// 共通変数
    float valid_length    = scene_view->sceneGraph()->lengthUnit().epsilonValidLength();
    float valid_length_sq = scene_view->sceneGraph()->lengthUnit().epsilonValidLengthSquared();

    if ((new_point0 - new_point1).length2() <= valid_length_sq) {
        return false;
    }

    const auto& camera_dir   = scene_view->cameraDir(new_point1);
    const auto& camera_up    = scene_view->cameraUp(new_point1);
    const auto& camera_right = (camera_up ^ camera_dir).normalized();    // scene_view->cameraRight();

    /// 1点目の通る平面を特定
    ///  ※ VHDで延長のとき、この平面があれば２点目が決まるので先に実施

    struct OnPlaneInfo {
        int    m_clip_index;
        Planef m_plane;
        OnPlaneInfo(int index, const Planef& plane) : m_clip_index(index), m_plane(plane) {}
    };

    std::vector<OnPlaneInfo> point0_on_planes;

    int         point_on_plane_index      = -1;
    int         point_on_plane_list_index = -1;
    Planef      point_on_plane;
    auto        clipping_ctrl  = m_3DForm->clippingCtrl();
    const auto& current_planes = clipping_ctrl->currentValidPlanes(bbox);
    for (int ic = 0; ic < current_planes.size(); ++ic) {
        int         index         = current_planes[ic].first;
        const auto& current_plane = current_planes[ic].second;

        float dist_on_plane = fabs(current_plane.distanceToPoint(new_point0));
        if (dist_on_plane <= valid_length) {
            if (point_on_plane_index < 0) {
                point_on_plane_index      = index;
                point_on_plane_list_index = ic;
                point0_on_planes.emplace_back(index, current_plane);
            }
            else {
                const auto& norm0 = current_planes[point_on_plane_index].second.normal();
                const auto& norm1 = current_plane.normal();
                if (fabs(norm0 * camera_dir) < fabs(norm1 * camera_dir)) {
                    point_on_plane_index      = index;
                    point_on_plane_list_index = ic;
                }

                point0_on_planes.emplace_back(index, current_plane);
            }
        }
    }
    if (point_on_plane_list_index >= 0) {
        point_on_plane = current_planes[point_on_plane_list_index].second;
    }

    /// 単純なメッシュとの３次元交点が内外判定が微妙なので断面から求める

    /// Line方向
    Point3f line_dir = new_point1 - new_point0;
    line_dir.normalize();
    if (line_dir.length2() == 0.0) {
        return false;
    }

    /// Lineが乗る平面と、それに直交して断面を出す平面
    Planef clip_plane;
    Planef sect_plane;

    /// 平面上判定
    std::vector<OnPlaneInfo> point01_on_planes;
    for (auto& on_plane : point0_on_planes) {
        if (fabs(on_plane.m_plane.distanceToPoint(new_point1)) <= valid_length) {
            point01_on_planes.emplace_back(on_plane);
        }
    }
    if (point01_on_planes.size() == 1) {
        clip_plane = point01_on_planes[0].m_plane;
        sect_plane = Planef((clip_plane.normal() ^ line_dir).normalized(), new_point0);
    }
    else if (point01_on_planes.size() > 1) {
        Point3f norm;
        for (auto& on_plane : point01_on_planes) {
            norm += on_plane.m_plane.normal();
        }
        norm.normalize();
        /// 暫定対策 - 2D面表示を対応したため同一面２つが来て法線ゼロになる。
        if (norm.length2() == 0.0) {
            norm = point01_on_planes[0].m_plane.normal();
        }

        clip_plane = point01_on_planes[0].m_plane;
        sect_plane = Planef((norm ^ line_dir).normalized(), new_point0);
    }
    else {
        float prod_line_camera_0, prod_line_camera_1;

        Point3f axis       = camera_up;
        prod_line_camera_0 = fabs(line_dir * camera_up);

        prod_line_camera_1 = fabs(line_dir * camera_right);
        if (prod_line_camera_1 < prod_line_camera_0) {
            prod_line_camera_0 = prod_line_camera_1;
            axis               = camera_right;
        }
        prod_line_camera_1 = fabs(line_dir * camera_dir);
        if (prod_line_camera_1 < prod_line_camera_0) {
            prod_line_camera_0 = prod_line_camera_1;
            axis               = camera_dir;
        }

        Point3f axis2 = line_dir ^ axis;
        axis          = axis2 ^ line_dir;

        clip_plane = Planef(axis, new_point0);
        sect_plane = Planef(axis2, new_point0);
    }

    bool need_clipping = false;
    /// 平面を補正
    if (point_on_plane_index >= 0) {
        /// 2点目も同一平面に乗っている場合
        if (fabs(point_on_plane.distanceToPoint(new_point1)) <= valid_length) {
            /// すでにクリップ面があるのでクリップしないように法線ゼロにする（ゼロは除外している）
            if (clip_plane == point_on_plane) {
                clip_plane = Planef(Point3f(0, 0, 0), new_point0);
            }
        }
        else {
            need_clipping = true;
        }
    }
    else {
        need_clipping = true;
    }

    /// 必要ならクリッピングしたのち、その形状に対してさらに断面とって断面線取得
    std::vector<Clipping::SectionInfo> sections;

    BoundingBox3f clip_box;
    clip_box.expandBy(new_point0);
    clip_box.expandBy(new_point1);
    clip_box.expandBy(valid_length * 10.0f);

    Clipping clipping(scene_view->sceneGraph());
    clipping.sectionLineForAutoLength(clip_plane, sect_plane, true, sections, &clip_box);

    return addAutoDimensionInfo(scene_view, new_point0, new_point1, sections, nullptr, nullptr, false, false,
                                sorted_dimensions_info);
}

void DimensionCtrl::updateAutoDimensionOnSection(SceneView* scene_view, const BoundingBox3f& bbox, Node* target,
                                                 const VecPoint3f& base_point_on_planes, bool update)
{
    Point3f                                    last_point;
    std::vector<MultiDimension::DimensionInfo> sorted_dimensions_info;
    for (int point_count = 0; point_count < (int)base_point_on_planes.size() - 1; point_count += 2) {
        Point3f new_point0 = base_point_on_planes[point_count];
        Point3f new_point1 = base_point_on_planes[point_count + 1];

        std::vector<MultiDimension::DimensionInfo> dim_info;
        if (updateAutoDimensionOnSection(scene_view, bbox, new_point0, new_point1, dim_info)) {
            if (point_count > 0 && dim_info.size() > 0) {
                const auto& new_start = dim_info[0].m_dimension->posStart();
                if (new_start != last_point) {
                    std::vector<MultiDimension::DimensionInfo> dim_info2;
                    if (updateAutoDimensionOnSection(scene_view, bbox, last_point, new_start, dim_info2)) {
                        sorted_dimensions_info.insert(sorted_dimensions_info.end(), dim_info2.begin(), dim_info2.end());
                    }
                }
            }
            sorted_dimensions_info.insert(sorted_dimensions_info.end(), dim_info.begin(), dim_info.end());
        }

        if (sorted_dimensions_info.size() > 0) {
            last_point = sorted_dimensions_info[sorted_dimensions_info.size() - 1].m_dimension->posEnd();
        }
    }

    if (!sorted_dimensions_info.empty()) {
        RefPtr<MultiDimension> old_dim = (MultiDimension*)target->object();

        auto dimension = MultiDimension::createObject();

        dimension->addDimensions(sorted_dimensions_info);

        target->setObject(dimension.ptr());

        /// 断面沿いの更新時は変更しない。現状はオリジナルから作らないとおかしくなるので
        dimension->setBasePoints(old_dim->basePoints());
        dimension->setBasePointPlanes(old_dim->basePointPlanes());

        dimension->setTextAlign(old_dim->textAlign());
        dimension->setDisplayValue(old_dim->isDisplayValue());
        dimension->setDisplayMaterial(old_dim->isDisplayMaterial());
        dimension->setOnSectionPlane(old_dim->isOnSectionPlane());
        dimension->setLineWidth(old_dim->lineWidth());
        dimension->setColor(old_dim->color());
        dimension->setDispName(old_dim->dispName());
        dimension->setName(old_dim->name());
        dimension->setEdgeType(old_dim->edgeType());
        dimension->setDisplayUnit(old_dim->isDisplayUnit());

        dimension->updateText();
    }

    if (update) {
        ui->obj3dViewer->update();
    }
}

bool DimensionCtrl::createAutoDimensionByPoint(bool temprorary)
{
    Point3f point(ui->doubleSpinBox_create_auto_dimension_pos1_x->value() / 1e3f,
                  ui->doubleSpinBox_create_auto_dimension_pos1_y->value() / 1e3f,
                  ui->doubleSpinBox_create_auto_dimension_pos1_z->value() / 1e3f);

    if (temprorary) {
        auto dimension = MultiDimension::createObject();
        dimension->setTemporary(true);
        dimension->setExtendStart(true);
        dimension->setExtendEnd(true);

        const auto type = createAutoDimensionType();
        switch (type) {
            case CreateDimensionType::X:
                dimension->setOnSectionPlane(false);
                dimension->setTemporaryModeX();
                break;
            case CreateDimensionType::Y:
                dimension->setOnSectionPlane(false);
                dimension->setTemporaryModeY();
                break;
            case CreateDimensionType::Z:
                dimension->setOnSectionPlane(false);
                dimension->setTemporaryModeZ();
                break;
            case CreateDimensionType::Vertical:
                dimension->setOnSectionPlane(true);
                dimension->setTemporaryModeV();
                break;
            case CreateDimensionType::Horizontal:
                dimension->setOnSectionPlane(true);
                dimension->setTemporaryModeH();
                break;
            default:
                return false;
        }

        QColor temp_color;
        if (m_auto_dimension_color.lightnessF() >= 0.5) {
            temp_color = adjustColor(m_auto_dimension_color, -20);
        }
        else {
            temp_color = adjustColor(m_auto_dimension_color, 50);
        }
        dimension->setColor(temp_color.redF(), temp_color.greenF(), temp_color.blueF());

        dimension->setBasePoints({point});
        dimension->setLineWidth((float)ui->spinBox_create_auto_dimension_linewidth->value());

        auto temporary_root = ui->obj3dViewer->sceneView()->temporaryRootNode();

        temporary_root->removeChild(m_temporary_auto_preview_dimension.ptr());

        auto new_node = temporary_root->addChild();
        new_node->setObject(dimension.ptr());

        m_temporary_auto_preview_dimension = new_node;
        ui->obj3dViewer->update();
        return true;
    }
    else {
        PickData pick_data;
        pick_data.setPickPoint(point);

        const auto type = createAutoDimensionType();
        switch (type) {
            case CreateDimensionType::Vertical:
            case CreateDimensionType::Horizontal:
                ui->obj3dViewer->adjustPickData(pick_data, false);
                break;
            default:
                break;
        }

        clearTemporaryPreviewAutoDimension();

        createAutoDimension(ui->obj3dViewer->sceneView(), {pick_data.pickPoint()}, false, false, false);
        ui->obj3dViewer->update();
        return true;
    }
}

bool DimensionCtrl::isContinuousAutoDimension() const
{
    return ui->checkBox_create_auto_dimension_continue->isEnabled()
        && ui->checkBox_create_auto_dimension_continue->isChecked();
}

bool DimensionCtrl::isDuringContinuousAutoDimension() const
{
    if (m_continuous_auto_dimension == nullptr || m_continuous_auto_dimension->parent() == nullptr) {
        return false;
    }

    return true;
}

Point3f DimensionCtrl::continousAutoDimensionPoint() const
{
    if (m_continuous_auto_dimension != nullptr) {
        return ((MultiDimension*)m_continuous_auto_dimension->object())->posEnd();
    }
    return Point3f();
}

void DimensionCtrl::fixContinuousAutoDimension()
{
    /// 延長の場合最後をやり直して延長
    if (ui->checkBox_create_auto_dimension_extend->isChecked()) {
        if (m_continuous_auto_dimension != nullptr) {
            MultiDimension* dimension = (MultiDimension*)m_continuous_auto_dimension->object();

            Point3f start, end;
            if (dimension->removeLastLine(start, end)) {
                createAutoDimension(ui->obj3dViewer->sceneView(), {start, end}, false, true);
            }
        }
    }

    m_continuous_auto_dimension = nullptr;
}

void DimensionCtrl::clearTemporaryDimension()
{
    Node* temporary_root = ui->obj3dViewer->sceneView()->temporaryRootNode();
    if (m_temporary_dimension != nullptr) {
        temporary_root->removeChild(m_temporary_dimension.ptr());
    }

    m_temporary_dimension = nullptr;
}

void DimensionCtrl::clearTemporaryAutoDimension()
{
    Node* temporary_root = ui->obj3dViewer->sceneView()->temporaryRootNode();
    if (m_temporary_auto_dimension != nullptr) {
        temporary_root->removeChild(m_temporary_auto_dimension.ptr());
    }

    m_temporary_auto_dimension = nullptr;
}

void DimensionCtrl::clearTemporaryPreviewAutoDimension()
{
    Node* temporary_root = ui->obj3dViewer->sceneView()->temporaryRootNode();
    if (m_temporary_auto_preview_dimension != nullptr) {
        temporary_root->removeChild(m_temporary_auto_preview_dimension.ptr());
        ui->obj3dViewer->update();
    }

    m_temporary_auto_preview_dimension = nullptr;

    if (ui->pushButton_create_auto_dimension_pos_preview->isChecked()) {
        ui->pushButton_create_auto_dimension_pos_preview->blockSignals(true);
        ui->pushButton_create_auto_dimension_pos_preview->setChecked(false);
        ui->pushButton_create_auto_dimension_pos_preview->blockSignals(false);
    }
}

bool DimensionCtrl::temporaryDimensionLastPoint(Point3f& pos)
{
    if (m_temporary_dimension != nullptr) {
        // if (m_temporary_dimension.isAlive()) {
        Dimension* dim = (Dimension*)m_temporary_dimension->object();

        pos = dim->posEnd();
        return true;
        //}
    }
    return false;
}

bool DimensionCtrl::temporaryAutoDimensionLastPoint(Point3f& pos)
{
    if (m_temporary_auto_dimension != nullptr) {
        // if (m_temporary_auto_dimension.isAlive()) {
        MultiDimension* dim = (MultiDimension*)m_temporary_auto_dimension->object();

        const auto& base_point = dim->basePoints();
        if (base_point.size() > 1) {
            pos = base_point[1];
            return true;
        }
        //}
    }
    return false;
}

bool DimensionCtrl::temporaryDimensionPoint(Point3f& point0, Point3f& point1)
{
    if (m_temporary_dimension != nullptr) {
        // if (m_temporary_dimension.isAlive()) {
        Dimension* dim = (Dimension*)m_temporary_dimension->object();
        point0         = dim->posStart();
        point1         = dim->posEnd();
        return true;
        //}
    }
    return false;
}

bool DimensionCtrl::temporaryAutoDimensionPoint(Point3f& point0, Point3f& point1)
{
    if (m_temporary_auto_dimension != nullptr) {
        // if (m_temporary_auto_dimension.isAlive()) {
        MultiDimension* dim = (MultiDimension*)m_temporary_auto_dimension->object();

        const auto& base_point = dim->basePoints();
        if (base_point.size() > 1) {
            point0 = base_point[0];
            point1 = base_point[1];
            return true;
        }
        //}
    }
    return false;
}
