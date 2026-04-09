#include "ResultCtrl.h"

#include "ClippingCtrl.h"
#include "CsvUtil.h"
#include "Op2ArraySetting.h"
#include "ResultOutputDlg.h"
#include "SaveColorMapDlg.h"
#include "SettingCtrl.h"
#include "SettingGuiCtrl.h"
#include "UtilityCtrl.h"
#include "Vox3DForm.h"
#include "ui_Vox3DForm.h"

#include "lz4.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QStyledItemDelegate>

#include <execution>

#include "Scene/VoxelScalar.h"

#include "tinycolormap.hpp"

/// 暫定
#define RESULT2DLIST_ARRY_X 8
#define RESULT2DLIST_ARRY_Y 9
#define RESULT2DLIST_VOX_X 10
#define RESULT2DLIST_VOX_Y 11

/// E2 Viewer カラーマップ
const std::vector<Point4f> ResultCtrl::m_e2viewer_colormap = {
    Point4f(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0, 1.0f),    // white
    Point4f(255.0 / 255.0, 0, 0, 1.0f),                            // red
    Point4f(250.0 / 255.0, 120.0 / 255.0, 0, 1.0f),
    Point4f(255.0 / 255.0, 255.0 / 255.0, 0, 1.0f),               // yellow
    Point4f(173.0 / 255.0, 255.0 / 255.0, 47.0 / 255.0, 1.0f),    // greenyellow
    Point4f(0, 255.0 / 255.0, 0, 1.0f),                           // lime
    Point4f(124.0 / 255.0, 238.0 / 255.0, 124.0 / 255.0, 1.0f),
    Point4f(0, 255.0 / 255.0, 255.0 / 255.0, 1.0f),               // cyan
    Point4f(65.0 / 255.0, 105.0 / 255.0, 225.0 / 255.0, 1.0f),    // royalblue
    Point4f(0, 0, 255.0 / 255.0, 1.0f),                           // blue
    Point4f(0, 0, 0, 1.0f)                                        // black
};

const ResultCtrl::CustomColorInfo ResultCtrl::m_custom_color_info_empty;

/// イベントフィルタを使用してクリックイベントを処理
class ClickHandler : public QObject {
public:
    explicit ClickHandler(ResultCtrl* ctrl, QGraphicsScene* scene, QObject* parent = nullptr)
        : QObject(parent)
        , m_ctrl(ctrl)
        , m_scene(scene)
    {
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF      scenePos   = static_cast<QGraphicsView*>(obj)->mapToScene(mouseEvent->pos());

            // シーンからクリック位置のアイテムを取得
            QGraphicsRectItem* item = dynamic_cast<QGraphicsRectItem*>(m_scene->itemAt(scenePos, QTransform()));
            if (item) {
                QVariant index = item->data(0);    // 0 はカスタムプロパティのキー
                if (index.isValid()) {
                    int color_index = index.toInt();
                    // qDebug() << "Clicked Rect Index:" << color_index;

                    auto& colors = m_ctrl->colors();
                    if (colors.size() > color_index && color_index >= 0) {
                        QColor init_color(colors[color_index][0] * 255.0f, colors[color_index][1] * 255.0f,
                                          colors[color_index][2] * 255.0f, colors[color_index][3] * 255.0f);

                        QColor selectedColor = QColorDialog::getColor(init_color, (QWidget*)this->parent(), "色を選択");
                        if (selectedColor.isValid() && selectedColor != init_color) {
                            selectedColor.setAlphaF(1.0f);
                            item->setBrush(selectedColor);
                            m_ctrl->setCustomColor(color_index, Point4f(selectedColor.redF(), selectedColor.greenF(),
                                                                        selectedColor.blueF(), 1.0f));

                            m_ctrl->setColorBand(-1, -1, true);
                        }
                    }
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QGraphicsScene* m_scene;
    ResultCtrl*     m_ctrl;
};
class TextDialog : public QDialog {
public:
    TextDialog(const QList<QPair<QString, QString>>& items, const QString& title,
               const QString& label,    // ← ラベル文字列を追加
               QWidget*       parent = nullptr)
        : QDialog(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 先頭にラベルを追加
        QLabel* topLabel = new QLabel(label, this);
        topLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);    // 選択＆コピー可 // 枠線を消してラベル風に
        mainLayout->addWidget(topLabel);

        CustomTableWidget* tableWidget = new CustomTableWidget(items.size(), 2, this);

        // ヘッダー設定
        tableWidget->setHorizontalHeaderLabels(QStringList() << "item"
                                                             << "value");
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);    // 編集不可

        // 項目と値をテーブルに追加
        for (int row = 0; row < items.size(); ++row) {
            tableWidget->setItem(row, 0, new QTableWidgetItem(items[row].first));
            tableWidget->setItem(row, 1, new QTableWidgetItem(items[row].second));
        }

        // 列幅・行高を内容にフィット
        tableWidget->resizeColumnsToContents();
        tableWidget->resizeRowsToContents();

        mainLayout->addWidget(tableWidget);

        // Closeボタン（右下）
        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // ボタンを右下に配置
        mainLayout->addWidget(buttonBox, 0, Qt::AlignRight);

        setLayout(mainLayout);
        setWindowTitle(title);

        // テーブルの内容に合わせてサイズを計算
        int width = tableWidget->verticalHeader()->width();
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            width += tableWidget->columnWidth(col);
        }
        width += tableWidget->frameWidth() * 2;

        int height = tableWidget->horizontalHeader()->height();
        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            height += tableWidget->rowHeight(row);
        }
        height += tableWidget->frameWidth() * 2;

        if (tableWidget->verticalScrollBar()->isVisible()) width += tableWidget->verticalScrollBar()->width();
        if (tableWidget->horizontalScrollBar()->isVisible()) height += tableWidget->horizontalScrollBar()->height();

        height += buttonBox->sizeHint().height() + 20;    // 余白

        // ラベル分の高さも追加
        height += topLabel->sizeHint().height() + 10;

        this->resize(width + 50, height + 20);
    }
};

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

ResultCtrl::ResultCtrl(Vox3DForm* form, Ui::Vox3DForm* ui_) : m_3DForm(form), ui(ui_)
{
    /// Color設定
    auto& cl    = m_colormap_default_list;
    using Style = ColorMapStyle;
    using Type  = tinycolormap::ColormapType;

    cl << ColormapInfo{"--- User defined ---", "", Style::Separator, Type(0)};
    cl << ColormapInfo{"--- Rainbow like ---", "", Style::Separator, Type(0)};
    cl << ColormapInfo{E2VIEWER_NAME, "Same as E2Viewer", Style::Custom, Type(0)};    /// 関数で設定
    // cl << ColormapInfo{"E2Viewer", "Same as E2Viewer", Style::E2ViewerLike, Type(0)};
    cl << ColormapInfo{"Turbo", "High-contrast rainbow (blue to red)", Style::Turbo, Type::Turbo};
    cl << ColormapInfo{"Heat", "Smooth blue to red", Style::Heat, Type::Heat};
    cl << ColormapInfo{"HSV", "Rainbow (full hue cycle)", Style::HSV, Type::HSV};
    cl << ColormapInfo{"Jet", "Rainbow (blue to red)", Style::Jet, Type::Jet};

    // Sequential colormaps
    cl << ColormapInfo{"--- Perceptually uniform ---", "", Style::Separator, Type(0)};
    cl << ColormapInfo{"Viridis", "Smooth blue to yellow", Style::Viridis, Type::Viridis};
    cl << ColormapInfo{"Cividis", "Colorblind-friendly blue to yellow", Style::Cividis, Type::Cividis};
    cl << ColormapInfo{"Plasma", "Smooth purple to yellow", Style::Plasma, Type::Plasma};
    cl << ColormapInfo{"Inferno", "Smooth dark to bright", Style::Inferno, Type::Inferno};
    cl << ColormapInfo{"Magma", "Smooth dark to light", Style::Magma, Type::Magma};

    cl << ColormapInfo{"--- White to color ---", "", Style::Separator, Type(0)};
    cl << ColormapInfo{"W to Black", "", Style::WhiteToBlack, Type(0)};
    cl << ColormapInfo{"W to Red", "", Style::WhiteToRed, Type(0)};
    cl << ColormapInfo{"W to Green", "", Style::WhiteToGreen, Type(0)};
    cl << ColormapInfo{"W to Blue", "", Style::WhiteToBlue, Type(0)};
    cl << ColormapInfo{"W to Cyan", "", Style::WhiteToCyan, Type(0)};
    cl << ColormapInfo{"W to Masenta", "", Style::WhiteToMagenta, Type(0)};
    cl << ColormapInfo{"W to Yellow", "", Style::WhiteToYellow, Type(0)};

    cl << ColormapInfo{"--- Color to black ---", "", Style::Separator, Type(0)};
    cl << ColormapInfo{"Red to B", "", Style::RedToBlack, Type(0)};
    cl << ColormapInfo{"Green to B", "", Style::GreenToBlack, Type(0)};
    cl << ColormapInfo{"Blue to B", "", Style::BlueToBlack, Type(0)};
    cl << ColormapInfo{"Cyan to B", "", Style::CyanToBlack, Type(0)};
    cl << ColormapInfo{"Magenta to B", "", Style::MagentaToBlack, Type(0)};
    cl << ColormapInfo{"Yellow to B", "", Style::YellowToBlack, Type(0)};

    cl << ColormapInfo{"--- Other sequential ---", "", Style::Separator, Type(0)};
    cl << ColormapInfo{"Parula", "Smooth blue to yellow", Style::Parula, Type::Parula};
    cl << ColormapInfo{"Cubehelix", "Monotonic grayscale", Style::Cubehelix, Type::Cubehelix};
    cl << ColormapInfo{"Hot", "Black to red to yellow to white", Style::Hot, Type::Hot};

    QStandardItemModel* model = new QStandardItemModel(ui->comboBox_result3d_custom_setting);
    ui->comboBox_result3d_custom_setting->setModel(model);
    for (int ic = 0; ic < cl.size(); ++ic) {
        QStandardItem* standardItem = new QStandardItem(cl[ic].m_name);
        standardItem->setData(cl[ic].m_tooltip, Qt::ToolTipRole);
        standardItem->setData(ic, Qt::UserRole + 1);
        standardItem->setData(0, Qt::UserRole + 2);    /// 0:標準,1:保存済,2:メモリ上編集
        if (cl[ic].m_style == Style::Separator) {
            standardItem->setFlags(Qt::NoItemFlags);    // 選択不可
        }
        model->appendRow(standardItem);
    }

    setE2ViewerColorStyle();
    ui->comboBox_result3d_custom_setting->setCurrentText(E2VIEWER_NAME);
    ui->comboBox_result3d_custom_setting->setMaxVisibleItems(cl.size() + 8);

    connect(ui->checkBox_result3d_view, &QCheckBox::toggled, this, [this]() {
        ui->obj3dViewer->showOpt3D(ui->checkBox_result3d_view->isChecked(), false);
        if (m_voxel_3d != nullptr) {
            /// 投影なければ何もしない（念のため抑制解除）
            if (!ui->obj3dViewer->hasProjectOpt(false)) {
                if (m_voxel_3d->isSuppress()) {
                    m_voxel_3d->unsuppress();
                }
            }
            else {
                if (!ui->checkBox_result3d_hide_on_proj->isChecked()) {
                    if (!m_voxel_3d->isSuppress()) {
                        m_voxel_3d->suppress();
                    }
                }
                else {
                    if (m_voxel_3d->isSuppress()) {
                        m_voxel_3d->unsuppress();
                    }
                }
            }
            /// クリッピング更新
            m_3DForm->clippingCtrl()->updateClipping(true);
            ui->obj3dViewer->update();
        }
    });
    connect(ui->LE_result3d_min_value, &QLineEdit::editingFinished, this, [this]() { setColorBand(); });
    connect(ui->LE_result3d_max_value, &QLineEdit::editingFinished, this, [this]() { setColorBand(); });
    connect(ui->rb_result3d_cmode_LINEAR, &QRadioButton::toggled, this, [this]() { setColorBand(); });
    connect(ui->comboBox_result3d_custom_setting, &QComboBox::currentIndexChanged, this, [this]() { setColorStyle(); });
    connect(ui->spinBox_result3d_color_count, &QSpinBox::valueChanged, this, [this]() {
        appendEditColorStyle(true);
        setColorBand(-1, -1, true);
    });
    connect(ui->checkBox_result3d_color_gradient, &QCheckBox::toggled, this, [this]() {
        appendEditColorStyle(true);
        setColorBand(-1, -1, true);
    });

    connect(ui->pushButton_result3d_set_max, &QPushButton::clicked, this, [this]() {
        if (m_result_all_max > 0) {
            setColorBand(-1, m_result_all_max);
        }
    });
    connect(ui->pushButton_result3d_set_min, &QPushButton::clicked, this, [this]() {
        if (m_result_all_min > 0 && m_result_all_min != FLT_MAX) {
            setColorBand(m_result_all_min, -1);
        }
    });
    connect(ui->doubleSpinBox_result3d_color_range_min, &QDoubleSpinBox::valueChanged, this, [this]() {
        appendEditColorStyle(true);
        setColorBand(-1, -1, true);
    });
    connect(ui->doubleSpinBox_result3d_color_range_max, &QDoubleSpinBox::valueChanged, this, [this]() {
        appendEditColorStyle(true);
        setColorBand(-1, -1, true);
    });
    connect(ui->checkBox_result3d_color_reverse, &QCheckBox::toggled, this, [this]() {
        appendEditColorStyle(true);
        setColorBand(-1, -1, true);
    });

    connect(ui->pushButton_result3d_custom_setting_save, &QPushButton::clicked, this, [this]() { saveColorStyle(); });
    connect(ui->pushButton_result3d_custom_setting_delete, &QPushButton::clicked, this,
            [this]() { deleteColorStyle(); });
    connect(ui->pushButton_result3d_custom_setting_update, &QPushButton::clicked, this, [this]() { loadColorStyle(); });

    connect(ui->pushButton_result3d_opt_detail, &QPushButton::clicked, this, [this]() { opt3dDetail(); });

    connect(ui->checkBox_result3d_display_in_view, &QCheckBox::toggled, this, [this]() {
        ui->obj3dViewer->set3DTextureColor(m_threshold_values, m_color_values);
        ui->obj3dViewer->setColorLabelCount(colorLabelCount());

        ui->obj3dViewer->setShowColorBar(ui->checkBox_result3d_display_in_view->isChecked());
        ui->obj3dViewer->update();
    });

    ui->doubleSpinBox_result3d_show_information_on_click->setEnabled(
        ui->checkBox_result3d_show_information_on_click->isChecked());
    ui->label_result3d_show_information_on_click_msec->setEnabled(
        ui->checkBox_result3d_show_information_on_click->isChecked());
    connect(ui->checkBox_result3d_show_information_on_click, &QCheckBox::toggled, this, [this]() {
        ui->doubleSpinBox_result3d_show_information_on_click->setEnabled(
            ui->checkBox_result3d_show_information_on_click->isChecked());
        ui->label_result3d_show_information_on_click_msec->setEnabled(
            ui->checkBox_result3d_show_information_on_click->isChecked());
    });

    connect(ui->checkBox_result3d_hide_on_proj, &QCheckBox::toggled, this, [this]() {
        if (m_voxel_3d != nullptr) {
            /// 投影なければ何もしない（念のため抑制解除）
            if (!ui->obj3dViewer->hasProjectOpt(false)) {
                if (m_voxel_3d->isSuppress()) {
                    m_voxel_3d->unsuppress();
                    /// クリッピング更新
                    m_3DForm->clippingCtrl()->updateClipping(true);
                    ui->obj3dViewer->update();
                }
            }
            else {
                if (!ui->checkBox_result3d_hide_on_proj->isChecked()) {
                    if (!m_voxel_3d->isSuppress()) {
                        m_voxel_3d->suppress();
                        /// クリッピング更新
                        m_3DForm->clippingCtrl()->updateClipping(true);
                        ui->obj3dViewer->update();
                    }
                }
                else {
                    if (m_voxel_3d->isSuppress()) {
                        m_voxel_3d->unsuppress();
                        /// クリッピング更新
                        m_3DForm->clippingCtrl()->updateClipping(true);
                        ui->obj3dViewer->update();
                    }
                }
            }
        }
    });

    connect(ui->pushButton_result3d_set_to_colormap, &QPushButton::clicked, this, [this]() {
        if (m_voxel_3d != nullptr) {
            auto voxel_scalar_obj = m_voxel_3d->object<VoxelScalar>();
            if (voxel_scalar_obj) {
                float min_value = voxel_scalar_obj->minValue();
                float max_value = voxel_scalar_obj->maxValue();
                setColorBand(min_value, max_value);
            }
        }
    });

    ui->checkBox_result3d_display_priority->setChecked(true);
    ui->obj3dViewer->setVoxelScalarPriority(ui->checkBox_result3d_display_priority->isChecked());
    connect(ui->checkBox_result3d_display_priority, &QCheckBox::toggled, this, [this]() {
        ui->obj3dViewer->setVoxelScalarPriority(ui->checkBox_result3d_display_priority->isChecked());
        ui->obj3dViewer->update();
    });

    /// 暫定（初期表示）
    QPen outlinePen(Qt::transparent);
    outlinePen.setWidth(1);

    /// 最初にラベルをすべて作っておく/未使用は非表示
    for (int ic = 0; ic < 16; ++ic) {
        QLineEdit* label = new QLineEdit(ui->frame_result3d_color_range);

        m_threshold_labels << label;
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->setReadOnly(true);                 // 読み取り専用に設定
        label->setFrame(false);                   // フレームを非表示に設定
        label->setContentsMargins(0, 0, 0, 0);    // コンテンツのマージンを0に設定
        label->setStyleSheet("QLineEdit { background: transparent; border: none; }");
        label->setGeometry(40, 20, label->width(), 20);
        label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        label->hide();
    }

    auto scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 30, 240);
    ui->result3d_colorsampleView->setScene(scene);

    ClickHandler* clickHandler = new ClickHandler(this, scene);
    ui->result3d_colorsampleView->installEventFilter(clickHandler);
    // ui->resultTab->installEventFilter(clickHandler);

    /// カラー設定の読込み
    loadColorStyle();

    /// カラー設定の反映
    setColorMap();

    /// 範囲指定リスト
    create3dResultRangeList();
    add3dResultRangeList();

    /// 2D結果のタブ
    create2dReultTab();
}

ResultCtrl::~ResultCtrl() {}

QString ResultCtrl::appendEditColorStyle(bool no_color_change)
{
    if (m_suppress_set_color) {
        return "";
    }

    QStandardItemModel* model         = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    int                 current_index = ui->comboBox_result3d_custom_setting->currentIndex();
    if (current_index < 0 || current_index >= model->rowCount()) {
        return "";
    }

    auto color_item = model->item(current_index);
    if (!color_item) {
        return "";
    }

    /// 現在編集中
    int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
    if (is_custom == 2) {
        /// レンジが違う場合更新する
        int   color_count     = colorCount();
        auto& custom_colormap = m_custom_colormap[color_item->text()];

        if (!no_color_change && (custom_colormap.m_color_values.size() > 0)) {
            if ((custom_colormap.m_color_values.size() != color_count - 2)
                || (fabs(custom_colormap.m_range_min - ui->doubleSpinBox_result3d_color_range_min->value()) >= 1.0e-6f)
                || (fabs(custom_colormap.m_range_max - ui->doubleSpinBox_result3d_color_range_max->value())
                    >= 1.0e-6f)) {
                auto back_custom_colors = custom_colormap;

                custom_colormap.m_range_min = ui->doubleSpinBox_result3d_color_range_min->value();
                custom_colormap.m_range_max = ui->doubleSpinBox_result3d_color_range_max->value();

                float range = (custom_colormap.m_range_max - custom_colormap.m_range_min);
                if (fabs(range) < 1.0e-6f) {
                    range = 1.0e-6f;
                }

                custom_colormap.m_color_values.resize(color_count - 2);
                for (int i = 0; i < color_count - 2; ++i) {
                    float t = color_count > 3 ? static_cast<float>(i) / (color_count - 2 - 1) : 0;    // 0～1の範囲

                    t = range * t + custom_colormap.m_range_min;

                    custom_colormap.m_color_values[i] = customColor(t, back_custom_colors);
                }
            }
        }
        /// 色変更ありの場合、色変更の元がなければ作成する
        else if (!no_color_change && custom_colormap.m_color_values.size() == 0) {
            /// 現在の設定
            std::optional<tinycolormap::ColormapType> tiny_type;
            std::optional<QString>                    custom_name;
            colorMapStyle(tiny_type, custom_name);

            const auto& custom_colors =
                custom_name.has_value() ? m_custom_colormap[custom_name.value()] : m_custom_color_info_empty;

            int color_count = colorCount();

            custom_colormap.m_range_min = ui->doubleSpinBox_result3d_color_range_min->value();
            custom_colormap.m_range_max = ui->doubleSpinBox_result3d_color_range_max->value();

            float range = (custom_colormap.m_range_max - custom_colormap.m_range_min);
            if (fabs(range) < 1.0e-6f) {
                range = 1.0e-6f;
            }

            custom_colormap.m_color_values.resize(color_count - 2);
            for (int i = 0; i < color_count - 2; ++i) {
                float t = color_count > 3 ? static_cast<float>(i) / (color_count - 2 - 1) : 0;    // 0～1の範囲

                t = range * t + custom_colormap.m_range_min;

                switch (custom_colormap.m_original_style) {
                    case ColorMapStyle::E2ViewerLike:
                        custom_colormap.m_color_values[i] = e2ViewerColor(t);
                        break;
                    case ColorMapStyle::WhiteToBlack:
                    case ColorMapStyle::WhiteToRed:
                    case ColorMapStyle::WhiteToGreen:
                    case ColorMapStyle::WhiteToBlue:
                    case ColorMapStyle::WhiteToYellow:
                    case ColorMapStyle::WhiteToCyan:
                    case ColorMapStyle::WhiteToMagenta:
                        custom_colormap.m_color_values[i] = whiteToColor(t, custom_colormap.m_original_style);
                        break;
                    case ColorMapStyle::RedToBlack:
                    case ColorMapStyle::GreenToBlack:
                    case ColorMapStyle::BlueToBlack:
                    case ColorMapStyle::YellowToBlack:
                    case ColorMapStyle::CyanToBlack:
                    case ColorMapStyle::MagentaToBlack:
                        custom_colormap.m_color_values[i] = colorToBlack(t, custom_colormap.m_original_style);
                        break;
                    case ColorMapStyle::Custom:
                        custom_colormap.m_color_values[i] = customColor(t, custom_colors);
                        break;
                    default:
                        custom_colormap.m_color_values[i] = tinyColor(t, custom_colormap.m_original_tiny_style);
                        break;
                }
            }
        }

        if (custom_colormap.m_max_color.has_value()) {
            custom_colormap.m_max_color = m_color_max;
        }
        if (custom_colormap.m_min_color.has_value()) {
            custom_colormap.m_min_color = m_color_min;
        }
        if (custom_colormap.m_save_range_max.has_value()) {
            custom_colormap.m_save_range_max = ui->doubleSpinBox_result3d_color_range_max->value();
        }
        if (custom_colormap.m_save_range_min.has_value()) {
            custom_colormap.m_save_range_min = ui->doubleSpinBox_result3d_color_range_min->value();
        }
        if (custom_colormap.m_save_gradient.has_value()) {
            custom_colormap.m_save_gradient = ui->checkBox_result3d_color_gradient->isChecked();
        }
        if (custom_colormap.m_save_reverse.has_value()) {
            custom_colormap.m_save_reverse = ui->checkBox_result3d_color_reverse->isChecked();
        }
        if (custom_colormap.m_save_steps.has_value()) {
            custom_colormap.m_save_steps = ui->spinBox_result3d_color_count->value();
        }

        return color_item->text();
    }

    /// 現在の設定
    std::optional<tinycolormap::ColormapType> tiny_type;
    std::optional<QString>                    custom_name;
    const auto&                               color_style = colorMapStyle(tiny_type, custom_name);

    /// 色変更なしの場合
    if (no_color_change) {
        /// 標準なら編集中にしない
        if (is_custom == 0) {
            return "";
        }
        else if (is_custom == 1) {
            /// 保存対象が編集されているか
            bool edit = false;
            if (custom_name.has_value()) {
                auto& color_info = m_custom_colormap[custom_name.value()];

                if (color_info.m_max_color.has_value()) {
                    if ((color_info.m_max_color.value() - m_color_max).length2() >= 1.0e-12f) {
                        edit = true;
                    }
                }
                if (color_info.m_min_color.has_value()) {
                    if ((color_info.m_min_color.value() - m_color_min).length2() >= 1.0e-12f) {
                        edit = true;
                    }
                }

                if (color_info.m_save_range_max.has_value()) {
                    if (fabs(color_info.m_save_range_max.value() - ui->doubleSpinBox_result3d_color_range_max->value())
                        < 1.0e-6f) {
                        edit = true;
                    }
                }
                if (color_info.m_save_range_min.has_value()) {
                    if (fabs(color_info.m_save_range_min.value() - ui->doubleSpinBox_result3d_color_range_min->value())
                        < 1.0e-6f) {
                        edit = true;
                    }
                }
                if (color_info.m_save_gradient.has_value()) {
                    if (color_info.m_save_gradient.value() != ui->checkBox_result3d_color_gradient->isChecked()) {
                        edit = true;
                    }
                }
                if (color_info.m_save_reverse.has_value()) {
                    if (color_info.m_save_reverse.value() != ui->checkBox_result3d_color_reverse->isChecked()) {
                        edit = true;
                    }
                }
                if (color_info.m_save_steps.has_value()) {
                    if (color_info.m_save_steps.value() != ui->spinBox_result3d_color_count->value()) {
                        edit = true;
                    }
                }
            }
            if (!edit) {
                return "";
            }
        }
    }

    QString name = ui->comboBox_result3d_custom_setting->currentText() + "*";

    /// 編集中が存在する
    int find_index = ui->comboBox_result3d_custom_setting->findText(name);
    if (find_index >= 0) {
        ui->comboBox_result3d_custom_setting->blockSignals(true);
        ui->comboBox_result3d_custom_setting->setCurrentIndex(find_index);
        ui->comboBox_result3d_custom_setting->blockSignals(false);
    }
    else {
        QStandardItem* standardItem = new QStandardItem(name);
        standardItem->setData(2, Qt::UserRole + 2);    /// 0:標準,1:保存済,2:メモリ上編集
        model->insertRow(current_index + 1, standardItem);

        ui->comboBox_result3d_custom_setting->blockSignals(true);
        ui->comboBox_result3d_custom_setting->setCurrentIndex(current_index + 1);
        ui->comboBox_result3d_custom_setting->blockSignals(false);
    }

    auto& custom_colormap                 = m_custom_colormap[name];
    custom_colormap.m_original_style      = color_style;
    custom_colormap.m_original_tiny_style = tiny_type.has_value() ? tiny_type.value() : (tinycolormap::ColormapType)0;
    if (!no_color_change) {
        custom_colormap.m_range_min = ui->doubleSpinBox_result3d_color_range_min->value();
        custom_colormap.m_range_max = ui->doubleSpinBox_result3d_color_range_max->value();
    }

    /// カスタム保存済みのとき
    if (custom_name.has_value()) {
        auto& color_info                      = m_custom_colormap[custom_name.value()];
        custom_colormap.m_original_style      = color_info.m_original_style;
        custom_colormap.m_original_tiny_style = color_info.m_original_tiny_style;
        custom_colormap.m_color_values        = color_info.m_color_values;

        if (color_info.m_max_color.has_value()) {
            custom_colormap.m_max_color = m_color_max;
        }
        if (color_info.m_min_color.has_value()) {
            custom_colormap.m_min_color = m_color_min;
        }
        if (color_info.m_save_range_max.has_value()) {
            custom_colormap.m_save_range_max = ui->doubleSpinBox_result3d_color_range_max->value();
        }
        if (color_info.m_save_range_min.has_value()) {
            custom_colormap.m_save_range_min = ui->doubleSpinBox_result3d_color_range_min->value();
        }
        if (color_info.m_save_gradient.has_value()) {
            custom_colormap.m_save_gradient = ui->checkBox_result3d_color_gradient->isChecked();
        }
        if (color_info.m_save_reverse.has_value()) {
            custom_colormap.m_save_reverse = ui->checkBox_result3d_color_reverse->isChecked();
        }
        if (color_info.m_save_steps.has_value()) {
            custom_colormap.m_save_steps = ui->spinBox_result3d_color_count->value();
        }
    }

    /// 色変更しない
    if (no_color_change) {
        return name;
    }

    auto custom_colors = custom_name.has_value() ? m_custom_colormap[custom_name.value()] : m_custom_color_info_empty;

    int color_count = colorCount();

    float range = (custom_colormap.m_range_max - custom_colormap.m_range_min);
    if (fabs(range) < 1.0e-6f) {
        range = 1.0e-6f;
    }

    custom_colormap.m_color_values.resize(color_count - 2);
    for (int i = 0; i < color_count - 2; ++i) {
        float t = color_count > 3 ? static_cast<float>(i) / (color_count - 2 - 1) : 0;    // 0～1の範囲

        t = range * t + custom_colormap.m_range_min;

        if (custom_name.has_value()) {
            custom_colormap.m_color_values[i] = customColor(t, custom_colors);
            continue;
        }

        switch (custom_colormap.m_original_style) {
            case ColorMapStyle::E2ViewerLike:
                custom_colormap.m_color_values[i] = e2ViewerColor(t);
                break;
            case ColorMapStyle::WhiteToBlack:
            case ColorMapStyle::WhiteToRed:
            case ColorMapStyle::WhiteToGreen:
            case ColorMapStyle::WhiteToBlue:
            case ColorMapStyle::WhiteToYellow:
            case ColorMapStyle::WhiteToCyan:
            case ColorMapStyle::WhiteToMagenta:
                custom_colormap.m_color_values[i] = whiteToColor(t, custom_colormap.m_original_style);
                break;
            case ColorMapStyle::RedToBlack:
            case ColorMapStyle::GreenToBlack:
            case ColorMapStyle::BlueToBlack:
            case ColorMapStyle::YellowToBlack:
            case ColorMapStyle::CyanToBlack:
            case ColorMapStyle::MagentaToBlack:
                custom_colormap.m_color_values[i] = colorToBlack(t, custom_colormap.m_original_style);
                break;
            case ColorMapStyle::Custom:
                custom_colormap.m_color_values[i] = customColor(t, custom_colors);
                break;
            default:
                custom_colormap.m_color_values[i] = tinyColor(t, custom_colormap.m_original_tiny_style);
                break;
        }
    }

    return name;
}

void ResultCtrl::appendCustomColorStyle(const QString& name, const CustomColorInfo& custom_color_info, bool set_current,
                                        int insert_index)
{
    using Style = ColorMapStyle;
    using Type  = tinycolormap::ColormapType;

    auto model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();

    if (insert_index < 0) {
        /// とりえあず名前順で

        /// Customのセクションの範囲
        for (int index = 1; index < model->rowCount(); ++index) {
            auto color_item = model->item(index);
            if (color_item) {
                int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
                if (is_custom == 1 || is_custom == 2) {
                    int compare = name.compare(color_item->text(), Qt::CaseInsensitive);
                    /// 挿入位置
                    if (compare > 0) {
                        if (insert_index < 0) {
                            insert_index = index + 1;
                        }
                    }
                    /// 上書き
                    else if (compare == 0) {
                        m_custom_colormap[name] = custom_color_info;
                        if (set_current) {
                            ui->comboBox_result3d_custom_setting->setCurrentIndex(index);
                        }
                        return;
                    }
                }
                else {
                    /// Customのセクションの範囲外（セパレーターで0)で抜ける
                    insert_index = index;
                    break;
                }
            }
        }
    }

    if (insert_index <= 0) insert_index = 1;

    m_custom_colormap[name] = custom_color_info;

    QStandardItem* standardItem = new QStandardItem(name);
    standardItem->setData(1, Qt::UserRole + 2);    /// 0:標準,1:保存済,2:メモリ上編集
    model->insertRow(insert_index, standardItem);

    if (set_current) {
        ui->comboBox_result3d_custom_setting->setCurrentIndex(insert_index);
    }
}

void ResultCtrl::setE2ViewerColorStyle()
{
    /// カラー以外も覚えるためカスタム（保存済み）で扱う（ただし削除は不可にする）
    /// そのための設定
    auto& color_info = m_custom_colormap[E2VIEWER_NAME];

    color_info.m_original_style = ColorMapStyle::E2ViewerLike;
    color_info.m_max_color      = Point4f(1.0f, 1.0f, 1.0f, 1.0f);
    color_info.m_min_color      = Point4f(0.0f, 0.0f, 0.0f, 1.0f);
    color_info.m_save_range_max = 1.0f;
    color_info.m_save_range_min = 0.0f;
    color_info.m_save_gradient  = false;
    color_info.m_save_reverse   = false;
    color_info.m_save_steps     = 9;
    color_info.m_range_min      = 0.0f;
    color_info.m_range_max      = 1.0f;

    auto model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();

    for (int index = 0; index < model->rowCount(); ++index) {
        auto color_item = model->item(index);
        if (color_item && color_item->text() == E2VIEWER_NAME) {
            /// カスタムの保存済み扱いにする
            color_item->setData(1, Qt::UserRole + 2);    /// 0:標準,1:保存済,2:メモリ上編集
            break;
        }
    }
}

void ResultCtrl::setResultStyleDefaultCombobox()
{
    const auto text = ui->comboBox_result3d_custom_setting_default->currentText();

    QStandardItemModel* custom_setting_default_model =
        new QStandardItemModel(ui->comboBox_result3d_custom_setting_default);
    ui->comboBox_result3d_custom_setting_default->setModel(custom_setting_default_model);

    bool                same_text            = false;
    QStandardItemModel* custom_setting_model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    for (int ic = 0; ic < ui->comboBox_result3d_custom_setting->count(); ++ic) {
        const auto& item_text = ui->comboBox_result3d_custom_setting->itemText(ic);

        if (!same_text) {
            if (text == item_text) {
                same_text = true;
            }
        }

        QStandardItem* standardItem = new QStandardItem(item_text);

        auto color_item = custom_setting_model->item(ic);
        int  is_custom  = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
        if (is_custom == 2) {
            continue;
        }
        standardItem->setFlags(custom_setting_model->item(ic)->flags());
        custom_setting_default_model->appendRow(standardItem);
    }

    if (same_text) {
        ui->comboBox_result3d_custom_setting_default->setCurrentText(text);
    }
    else {
        ui->comboBox_result3d_custom_setting_default->setCurrentText(E2VIEWER_NAME);
    }
}

void ResultCtrl::setCustomColor(int index, const Point4f& color)
{
    if (index == 0) {
        m_color_max = color;
        appendEditColorStyle(true);    /// Max設定している場合に編集中にする
        return;
    }
    else if (index == m_color_values.size() - 1) {
        m_color_min = color;
        appendEditColorStyle(true);    /// Min設定している場合に編集中にする
        return;
    }

    const auto& name = appendEditColorStyle();

    bool reverse = ui->checkBox_result3d_color_reverse->isChecked();

    auto& custom_colormap = m_custom_colormap[name];

    int value_index = reverse ? custom_colormap.m_color_values.size() - index : index - 1;
    if (0 <= value_index && value_index < custom_colormap.m_color_values.size()) {
        custom_colormap.m_color_values[value_index] = color;
    }
}

void ResultCtrl::setMinMaxLabel()
{
    m_result_all_min = FLT_MAX;
    m_result_all_max = -FLT_MAX;

    if (m_voxel_3d.isAlive()) {
        auto voxel_scalar = m_voxel_3d->object<VoxelScalar>();
        if (voxel_scalar) {
            m_result_all_min = std::min(m_result_all_min, voxel_scalar->minValue());
            m_result_all_max = std::max(m_result_all_max, voxel_scalar->maxValue());
        }
    }

    for (auto& node : m_voxel_2d_list) {
        if (node.isAlive()) {
            auto voxel_scalar = node->object<VoxelScalar>();
            if (voxel_scalar) {
                m_result_all_min = std::min(m_result_all_min, voxel_scalar->minValue());
                m_result_all_max = std::max(m_result_all_max, voxel_scalar->maxValue());
            }
        }
    }

    if (m_result_all_min != FLT_MAX) {
        ui->lb_result3d_max->setText("MAX (" + QString::number(m_result_all_max, 'E', 5) + ")");
        ui->lb_result3d_min->setText("MIN (" + QString::number(m_result_all_min, 'E', 5) + ")");
    }
    else {
        ui->lb_result3d_max->setText("MAX");
        ui->lb_result3d_min->setText("MIN");
    }

    /// ビュー上だと邪魔なのでやめておく
    // ui->obj3dViewer->setColorMinMaxLabel(ui->lb_result3d_min->text().toStdWString(),
    //                                      ui->lb_result3d_max->text().toStdWString());
}

Point4f ResultCtrl::tinyColor(float t, tinycolormap::ColormapType type, float min_range, float max_range)
{
    /// 必要なものは調整
    switch (type) {
        case tinycolormap::ColormapType::Gray:
            t = 1.0f - t;
            break;
    }

    t          = 1.0f - ((max_range - min_range) * t + min_range);
    auto color = tinycolormap::GetColor(t, type);
    return {(float)color.r(), (float)color.g(), (float)color.b(), 1.0f};
}

Point4f ResultCtrl::customColor(float t_org, const CustomColorInfo& custom_colorinfo)
{
    // t = (custom_colorinfo.m_range_max - custom_colorinfo.m_range_min) * t + custom_colorinfo.m_range_min;
    float diff = (custom_colorinfo.m_range_max - custom_colorinfo.m_range_min);
    if (fabs(diff) < 1.0e-6f) {
        diff = 1.0e-6f;
    }
    float t = (t_org - custom_colorinfo.m_range_min) / diff;

    // DEBUG() << "diff " << diff;
    // DEBUG() << "t " << t;
    // DEBUG() << "t_org " << t_org;

    const auto& custom_colormap = custom_colorinfo.m_color_values;

    int custom_color_size = (int)custom_colormap.size();
    if (custom_color_size == 0) {
        /// 色設定ない時はオリジナルスタイル
        switch (custom_colorinfo.m_original_style) {
            case ColorMapStyle::E2ViewerLike:
                return e2ViewerColor(t_org);
            case ColorMapStyle::WhiteToBlack:
            case ColorMapStyle::WhiteToRed:
            case ColorMapStyle::WhiteToGreen:
            case ColorMapStyle::WhiteToBlue:
            case ColorMapStyle::WhiteToYellow:
            case ColorMapStyle::WhiteToCyan:
            case ColorMapStyle::WhiteToMagenta:
                return whiteToColor(t_org, custom_colorinfo.m_original_style);
            case ColorMapStyle::RedToBlack:
            case ColorMapStyle::GreenToBlack:
            case ColorMapStyle::BlueToBlack:
            case ColorMapStyle::YellowToBlack:
            case ColorMapStyle::CyanToBlack:
            case ColorMapStyle::MagentaToBlack:
                return colorToBlack(t_org, custom_colorinfo.m_original_style);
            default:
                return tinyColor(t_org, custom_colorinfo.m_original_tiny_style);
        }
        return Point4f();
    }
    if (t == 0.0f) {
        return custom_colormap[0];
    }
    if (t == 1.0f) {
        return custom_colormap[custom_color_size - 1];
    }
    if ((t < 0.0f) || (t > 1.0f)) {
        switch (custom_colorinfo.m_original_style) {
            case ColorMapStyle::E2ViewerLike:
                return e2ViewerColor(t_org);
            case ColorMapStyle::WhiteToBlack:
            case ColorMapStyle::WhiteToRed:
            case ColorMapStyle::WhiteToGreen:
            case ColorMapStyle::WhiteToBlue:
            case ColorMapStyle::WhiteToYellow:
            case ColorMapStyle::WhiteToCyan:
            case ColorMapStyle::WhiteToMagenta:
                return whiteToColor(t_org, custom_colorinfo.m_original_style);
            case ColorMapStyle::RedToBlack:
            case ColorMapStyle::GreenToBlack:
            case ColorMapStyle::BlueToBlack:
            case ColorMapStyle::YellowToBlack:
            case ColorMapStyle::CyanToBlack:
            case ColorMapStyle::MagentaToBlack:
                return colorToBlack(t_org, custom_colorinfo.m_original_style);
            case ColorMapStyle::Custom:
                return t <= 0.0f ? custom_colormap[0] : custom_colormap[custom_color_size - 1];
            default:
                return tinyColor(t_org, custom_colorinfo.m_original_tiny_style);
        }
    }

    float          pos  = t * float(custom_color_size - 1);
    int            idx  = static_cast<int>(pos);
    float          frac = pos - idx;
    const Point4f& c0   = (idx >= 0 && idx < custom_color_size) ? custom_colormap[idx] : custom_colormap[0];
    const Point4f& c1   = (idx + 1 >= 0 && idx + 1 < custom_color_size) ? custom_colormap[idx + 1]
                                                                        : custom_colormap[custom_color_size - 1];

    // qDebug() << "idx " << idx;
    // qDebug() << "frac " << frac;

    return c0 * (1 - frac) + c1 * frac;
}

Point4f ResultCtrl::whiteToColor(float t, ColorMapStyle style)
{
    constexpr float adj1 = 0.2f;    /// 緑の明度調整
    constexpr float adj2 = 0.3f;    /// 黄の明度調整
    constexpr float adj3 = 0.2f;    /// シアンの明度調整

    t           = std::clamp(t, 0.0f, 1.0f);
    float value = 1.0f - t;

    switch (style) {
        case ColorMapStyle::WhiteToRed:
            return Point4f(1.0f, value, value, 1.0f);    /// 白→赤
        case ColorMapStyle::WhiteToGreen:
            return Point4f(value, 1.0f - adj1 * t, value, 1.0f);    /// 白→緑（明度調整）
        case ColorMapStyle::WhiteToBlue:
            return Point4f(value, value, 1.0f, 1.0f);    /// 白→青
        case ColorMapStyle::WhiteToYellow:
            return Point4f(1.0f - adj2 * t, 1.0f - adj2 * t, value, 1.0f);    /// 白→黄（明度調整）
        case ColorMapStyle::WhiteToCyan:
            return Point4f(value, 1.0f - adj3 * t, 1.0f - adj3 * t, 1.0f);    /// 白→シアン（明度調整）
        case ColorMapStyle::WhiteToMagenta:
            return Point4f(1.0f, value, 1.0f, 1.0f);    /// 白→マゼンタ
        case ColorMapStyle::WhiteToBlack:
            return Point4f(value, value, value, 1.0f);    /// 白→黒
        default:
            return Point4f(1.0f, 1.0f, 1.0f, 1.0f);    /// デフォルトは白
    }
}

Point4f ResultCtrl::colorToBlack(float t, ColorMapStyle style)
{
    constexpr float adj1 = 0.3f;    /// 赤の明度調整
    constexpr float adj2 = 0.2f;    /// 緑の明度調整
    constexpr float adj3 = 0.3f;    /// 青の明度調整
    constexpr float adj4 = 0.2f;    /// 黄の明度調整
    constexpr float adj5 = 0.2f;    /// シアンの明度調整
    constexpr float adj6 = 0.2f;    /// マゼンタの明度調整

    t           = std::clamp(t, 0.0f, 1.0f);
    float value = 1.0f - t;
    switch (style) {
        case ColorMapStyle::RedToBlack:
            return Point4f(value, value * adj1, value * adj1, 1.0f);    // 赤＋少し緑・青
        case ColorMapStyle::GreenToBlack:
            return Point4f(value * adj2, value, value * adj2, 1.0f);    // 緑＋少し赤・青
        case ColorMapStyle::BlueToBlack:
            return Point4f(value * adj3, value * adj3, value, 1.0f);    // 青＋少し赤・緑
        case ColorMapStyle::YellowToBlack:
            return Point4f(value, value, value * adj4, 1.0f);    // 黄＋少し青
        case ColorMapStyle::CyanToBlack:
            return Point4f(value * adj5, value, value, 1.0f);    // シアン＋少し赤
        case ColorMapStyle::MagentaToBlack:
            return Point4f(value, value * adj6, value, 1.0f);    // マゼンタ＋少し緑
        default:
            return Point4f(0.0f, 0.0f, 0.0f, 1.0f);    // 黒
    }
}

Point4f ResultCtrl::e2ViewerColor(float t)
{
    float scaled_t = std::clamp(t, 0.0f, 1.0f) * (m_e2viewer_colormap.size() - 2 - 1) + 1;

    int index1 = static_cast<int>(std::floor(scaled_t));
    int index2 = static_cast<int>(std::ceil(scaled_t));

    if (index1 == index2) {
        return m_e2viewer_colormap[index1];
    }

    float fraction = scaled_t - index1;

    return m_e2viewer_colormap[index1] + (m_e2viewer_colormap[index2] - m_e2viewer_colormap[index1]) * fraction;
}

void ResultCtrl::setThreshold(float min, float max, bool linear)
{
    int color_count = colorCount();

    m_threshold_values.clear();
    m_threshold_values.resize(color_count - 1);

    if (linear) {
        float d = max - min;
        for (int ic = 0; ic < color_count - 1; ++ic) {
            m_threshold_values[color_count - 2 - ic] = d * (1.0 / (double)(color_count - 2)) * (double)ic + min;
        }
    }
    else {
        /// こっちは入力制御していなければあり得る
        if (min <= 0) {
            min = std::numeric_limits<float>::min();
        }
        /// 不正防止
        if (max <= 0) {
            max = std::numeric_limits<float>::min();
        }
        double dl = log10(max) - log10(min);
        for (int ic = 0; ic < color_count - 1; ++ic) {
            m_threshold_values[color_count - 2 - ic] =
                pow(10.0, log10(min) + dl * (1.0 / (double)(color_count - 2)) * (double)ic);
            /// ゼロにする
            if (ic == 0 && min == std::numeric_limits<float>::min()) {
                m_threshold_values[color_count - 2 - ic] = 0;
            }
        }
    }
}

void ResultCtrl::setColorMap()
{
    setEnambleStyleSaveDelete();

    bool gradient = ui->checkBox_result3d_color_gradient->isChecked();

    bool  reverse   = ui->checkBox_result3d_color_reverse->isChecked();
    float range_min = ui->doubleSpinBox_result3d_color_range_min->value();
    float range_max = ui->doubleSpinBox_result3d_color_range_max->value();

    int color_count = colorCount();

    std::optional<tinycolormap::ColormapType> tiny_type;
    std::optional<QString>                    custom_name;
    const auto&                               color_style = colorMapStyle(tiny_type, custom_name);

    const auto& custom_colors =
        custom_name.has_value() ? m_custom_colormap[custom_name.value()] : m_custom_color_info_empty;

    float range = range_max - range_min;
    if (fabs(range) < 1.0e-6f) {
        range = 1.0e-6f;
    }

    m_color_values.resize(color_count);
    m_color_values[0] = m_color_max;
    for (int i = 0; i < color_count - 2; ++i) {
        float t = color_count > 3 ? static_cast<float>(i) / (color_count - 2 - 1) : 0;    // 0～1の範囲
        if (reverse) {
            t = 1 - t;
        }

        t = range * t + range_min;

        switch (color_style) {
            case ColorMapStyle::E2ViewerLike:
                m_color_values[i + 1] = e2ViewerColor(t);
                break;
            case ColorMapStyle::WhiteToBlack:
            case ColorMapStyle::WhiteToRed:
            case ColorMapStyle::WhiteToGreen:
            case ColorMapStyle::WhiteToBlue:
            case ColorMapStyle::WhiteToYellow:
            case ColorMapStyle::WhiteToCyan:
            case ColorMapStyle::WhiteToMagenta:
                m_color_values[i + 1] = whiteToColor(t, color_style);
                break;
            case ColorMapStyle::RedToBlack:
            case ColorMapStyle::GreenToBlack:
            case ColorMapStyle::BlueToBlack:
            case ColorMapStyle::YellowToBlack:
            case ColorMapStyle::CyanToBlack:
            case ColorMapStyle::MagentaToBlack:
                m_color_values[i + 1] = colorToBlack(t, color_style);
                break;
            case ColorMapStyle::Custom:
                m_color_values[i + 1] = customColor(t, custom_colors);
                break;
            default:
                if (tiny_type.has_value()) {
                    m_color_values[i + 1] = tinyColor(t, tiny_type.value());
                }
                break;
        }
    }

    m_color_values[color_count - 1] = m_color_min;

    QPen outlinePen(Qt::transparent);
    outlinePen.setWidth(1);

    /// uiと合わせる必要あり
    int scene_color_height       = 22;
    int scene_color_width        = 30;
    int scene_color_width_offset = 10;

    auto scene = ui->result3d_colorsampleView->scene();
    scene->clear();
    int   view_height     = scene->height() - scene_color_height * 2;
    float view_height_one = (float)view_height / (float)(m_color_values.size() - 2);
    for (int ic = 0; ic < color_count; ++ic) {
        QBrush             brush(QColor(m_color_values[ic][0] * 255.0f, m_color_values[ic][1] * 255.0f,
                                        m_color_values[ic][2] * 255.0f, m_color_values[ic][3] * 255.0f));
        QGraphicsRectItem* rect_item;
        if (ic == 0) {
            rect_item =
                scene->addRect(0, ic * scene_color_height, scene_color_width, scene_color_height, outlinePen, brush);
            rect_item->setData(0, ic);
        }
        else if (ic == color_count - 1) {
            rect_item = scene->addRect(0, scene->height() - scene_color_height, scene_color_width, scene_color_height,
                                       outlinePen, brush);
            rect_item->setData(0, ic);
        }
        else {
            float y0     = (float)(ic - 1) * view_height_one + scene_color_height;
            float y1     = (float)ic * view_height_one + scene_color_height;
            float height = y1 - y0;
            rect_item    = scene->addRect(0, y0, scene_color_width, height, outlinePen, brush);

            if (!gradient) {
                rect_item->setData(0, ic);
            }
        }
    }

    float step       = 1;
    int   loop_count = color_count - 1;

    if (colorLabelCount() != color_count) {
        loop_count = colorLabelCount() - 1;
        step       = (float)(color_count - 1 - 1) / (float)(loop_count - 1);
    }

    int label_count = 0;

    /// i==0とi==loop_count-1(両端)は通らないが、Renderのソースと合わせておく
    for (int i = 1; i < loop_count - 1; ++i) {
        int index = i;
        if (i != 0 && i != loop_count - 1) {
            index = (int)((float)step * (float)(i) + 0.5);
        }
        else if (i == loop_count - 1) {
            index = color_count - 1 - 1;
        }

        m_threshold_labels[label_count]->setText(QString::number(0.0, 'f', 5));
        m_threshold_labels[label_count]->setGeometry(
            scene_color_width + scene_color_width_offset,
            (index - 1) * view_height_one + scene_color_height + view_height_one,
            m_threshold_labels[label_count]->width(), m_threshold_labels[label_count]->height());
        m_threshold_labels[label_count]->show();
        m_threshold_labels[label_count]->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        ++label_count;
    }

    for (int ic = label_count; ic < m_threshold_labels.size(); ++ic) {
        m_threshold_labels[ic]->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_threshold_labels[ic]->hide();
    }
    ui->result3d_colorsampleView->update();
}

void ResultCtrl::setColorStyle()
{
    std::optional<tinycolormap::ColormapType> tiny_type;
    std::optional<QString>                    custom_name;
    colorMapStyle(tiny_type, custom_name);
    if (custom_name.has_value()) {
        m_suppress_set_color = true;

        auto& color_info = m_custom_colormap[custom_name.value()];
        if (color_info.m_max_color.has_value()) {
            m_color_max = color_info.m_max_color.value();
        }
        if (color_info.m_min_color.has_value()) {
            m_color_min = color_info.m_min_color.value();
        }
        if (color_info.m_save_steps.has_value()) {
            ui->spinBox_result3d_color_count->blockSignals(true);
            ui->spinBox_result3d_color_count->setValue(color_info.m_save_steps.value());
            ui->spinBox_result3d_color_count->blockSignals(false);
        }
        if (color_info.m_save_range_max.has_value()) {
            ui->doubleSpinBox_result3d_color_range_max->blockSignals(true);
            ui->doubleSpinBox_result3d_color_range_max->setValue(color_info.m_save_range_max.value());
            ui->doubleSpinBox_result3d_color_range_max->blockSignals(false);
        }
        if (color_info.m_save_range_min.has_value()) {
            ui->doubleSpinBox_result3d_color_range_min->blockSignals(true);
            ui->doubleSpinBox_result3d_color_range_min->setValue(color_info.m_save_range_min.value());
            ui->doubleSpinBox_result3d_color_range_min->blockSignals(false);
        }
        if (color_info.m_save_gradient.has_value()) {
            ui->checkBox_result3d_color_gradient->blockSignals(true);
            ui->checkBox_result3d_color_gradient->setChecked(color_info.m_save_gradient.value());
            ui->checkBox_result3d_color_gradient->blockSignals(false);
        }
        if (color_info.m_save_reverse.has_value()) {
            ui->checkBox_result3d_color_reverse->blockSignals(true);
            ui->checkBox_result3d_color_reverse->setChecked(color_info.m_save_reverse.value());
            ui->checkBox_result3d_color_reverse->blockSignals(false);
        }

        m_suppress_set_color = false;
    }

    setColorBand(-1, -1, true);
}

void ResultCtrl::loadColorStyle()
{
    int  current_index        = ui->comboBox_result3d_custom_setting->currentIndex();
    bool current_index_change = false;

    QString current_text;

    /// 保存済みを一旦全削除
    auto model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    for (int index = model->rowCount() - 1; index >= 0; --index) {
        auto color_item = model->item(index);
        if (color_item) {
            if (color_item->text() == E2VIEWER_NAME) {
                continue;
            }
            int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
            if (is_custom == 1) {
                m_custom_colormap.erase(color_item->text());

                if (index == current_index) {
                    if (!current_index_change) {
                        current_text         = color_item->text();
                        current_index_change = true;
                    }

                    ui->comboBox_result3d_custom_setting->blockSignals(true);
                    if (index > 1) {
                        ui->comboBox_result3d_custom_setting->setCurrentIndex(index - 1);
                        current_index = index - 1;
                    }
                    else if (index == 1) {
                        ui->comboBox_result3d_custom_setting->setCurrentText(E2VIEWER_NAME);
                        current_index = -1;
                    }
                    ui->comboBox_result3d_custom_setting->blockSignals(false);
                }
                ui->comboBox_result3d_custom_setting->blockSignals(true);
                ui->comboBox_result3d_custom_setting->removeItem(index);
                ui->comboBox_result3d_custom_setting->blockSignals(false);
            }
        }
    }

    std::set<QString> exist_file_name;
    /// ファイル名
    QDir        color_map_dir(SettingCtrl::resultColorMapPath());
    QStringList fileList = color_map_dir.entryList(QStringList() << "*.csv", QDir::Files | QDir::NoDotAndDotDot);
    for (QString fileName : fileList) {
        if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
            fileName.chop(4);
        }
        exist_file_name.insert(fileName);
    }

    std::set<QString> exist_name;
    for (auto& color_map : m_colormap_default_list) {
        exist_name.insert(color_map.m_name.toLower());
    }

    int insert_index = 1;

    bool has_current = false;

    for (auto& file_name : exist_file_name) {
        /// 不正なファイル名（標準名）は除外
        QString file_name_lower = file_name;
        if (exist_name.contains(file_name_lower.toLower())) {
            continue;
        }

        QList<QStringList> read_data;
        CsvUtil::readCsv(SettingCtrl::resultColorMapPath() + "/" + file_name + ".csv", read_data);

        CustomColorInfo custom_color_info;

        /// @Version,1
        /// @OriginalStyle,name
        /// @MaxColor,R,G,B
        /// @MinColor,R,G,B
        /// @Steps,number,
        /// @Gradient,
        /// @Range,min,max
        /// @Reverse,
        /// @CurRange,range_min,range_max
        /// @Color
        /// R,G,B
        /// R,G,B

        // QList<QStringList> invalid_lines;

        bool color_start = false;
        for (auto& data : read_data) {
            if (data.size() == 0) {
                continue;
            }

            /// ヘッダ情報
            if (data.size() > 0) {
                if (data[0].compare("@Color", Qt::CaseInsensitive) == 0) {
                    color_start = true;
                    continue;
                }
            }
            if (data.size() > 1) {
                if (data[0].compare("@OriginalStyle", Qt::CaseInsensitive) == 0) {
                    /// 標準のカラーマップ
                    if (data[1] == E2VIEWER_NAME) {
                        custom_color_info.m_original_style = ColorMapStyle::E2ViewerLike;
                    }
                    else {
                        for (auto& info : m_colormap_default_list) {
                            if (data[1] == info.m_name) {
                                custom_color_info.m_original_style      = info.m_style;
                                custom_color_info.m_original_tiny_style = info.m_tiny_type;
                                break;
                            }
                        }
                    }
                    continue;
                }
                else if (data[0].compare("@MaxColor", Qt::CaseInsensitive) == 0) {
                    if (data.size() > 3) {
                        if (!data[1].isEmpty() && !data[2].isEmpty() && !data[3].isEmpty()) {
                            custom_color_info.m_max_color =
                                Point4f(data[1].toFloat() / 255.0f, data[2].toFloat() / 255.0f,
                                        data[3].toFloat() / 255.0f, 1.0f);
                        }
                    }
                    continue;
                }
                else if (data[0].compare("@MinColor", Qt::CaseInsensitive) == 0) {
                    if (data.size() > 3) {
                        if (!data[1].isEmpty() && !data[2].isEmpty() && !data[3].isEmpty()) {
                            custom_color_info.m_min_color =
                                Point4f(data[1].toFloat() / 255.0f, data[2].toFloat() / 255.0f,
                                        data[3].toFloat() / 255.0f, 1.0f);
                        }
                    }
                    continue;
                }
                else if (data[0].compare("@Steps", Qt::CaseInsensitive) == 0) {
                    if (!data[1].isEmpty()) {
                        custom_color_info.m_save_steps =
                            std::clamp(data[1].toInt(), ui->spinBox_result3d_color_count->minimum(),
                                       ui->spinBox_result3d_color_count->maximum());
                    }
                    continue;
                }
                else if (data[0].compare("@Gradient", Qt::CaseInsensitive) == 0) {
                    if (!data[1].isEmpty()) {
                        custom_color_info.m_save_gradient = data[1].toInt() != 0 ? true : false;
                    }
                    continue;
                }
                else if (data[0].compare("@Range", Qt::CaseInsensitive) == 0) {
                    if (!data[1].isEmpty()) {
                        custom_color_info.m_save_range_min = std::clamp(data[1].toFloat(), 0.0f, 1.0f);
                    }
                    if (data.size() > 2) {
                        if (!data[2].isEmpty()) {
                            custom_color_info.m_save_range_max = std::clamp(data[2].toFloat(), 0.0f, 1.0f);
                        }
                    }
                    continue;
                }
                else if (data[0].compare("@Reverse", Qt::CaseInsensitive) == 0) {
                    if (!data[1].isEmpty()) {
                        custom_color_info.m_save_reverse = data[1].toInt() != 0 ? true : false;
                    }
                    continue;
                }
                else if (data[0].compare("@CurRange", Qt::CaseInsensitive) == 0) {
                    if (!data[1].isEmpty()) {
                        custom_color_info.m_range_min = std::clamp(data[1].toFloat(), 0.0f, 1.0f);
                    }
                    if (data.size() > 2) {
                        if (!data[2].isEmpty()) {
                            custom_color_info.m_range_max = std::clamp(data[2].toFloat(), 0.0f, 1.0f);
                        }
                    }
                    continue;
                }
            }

            /// コメント行はスキップ
            if (data[0].indexOf("#") == 0) {
                continue;
            }
            if (data[0].indexOf("@") == 0) {
                continue;
            }

            if (color_start) {
                if (data.size() > 2) {
                    if (!data[0].isEmpty() && !data[1].isEmpty() && !data[2].isEmpty()) {
                        custom_color_info.m_color_values.emplace_back(Point4f(
                            data[0].toFloat() / 255.0f, data[1].toFloat() / 255.0f, data[2].toFloat() / 255.0f, 1.0f));
                    }
                }
            }
            // else {
            //     if (data.size() > 2) {
            //         if (!data[0].isEmpty() && !data[1].isEmpty() && !data[2].isEmpty()) {
            //             invalid_lines << data;
            //         }
            //     }
            // }
        }

        /// 妥当か
        bool valid = false;
        if (custom_color_info.m_original_style == ColorMapStyle::Custom) {
            if (custom_color_info.m_color_values.size() > 0) {
                valid = true;
            }
        }
        else {
            valid = true;
        }
        if (valid) {
            if (current_text.compare(file_name, Qt::CaseInsensitive) == 0) {
                has_current  = true;
                current_text = file_name;
            }
            /// 追加
            appendCustomColorStyle(file_name, custom_color_info, false, insert_index);
            ++insert_index;
        }
    }

    if (current_index_change) {
        if (has_current) {
            ui->comboBox_result3d_custom_setting->setCurrentText(current_text);
        }
        else {
            setColorBand(-1, -1, true);
        }
    }
}

void ResultCtrl::saveColorStyle()
{
    int index = ui->comboBox_result3d_custom_setting->currentIndex();

    auto model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    if (index >= 0 && index < model->rowCount()) {
        auto color_item = model->item(index);
        if (color_item) {
            CustomColorInfo custom_colormap;
            int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
            if (is_custom == 2 || is_custom == 1) {
                custom_colormap = m_custom_colormap[color_item->text()];
            }
            else {
                std::optional<tinycolormap::ColormapType> tiny_type;
                std::optional<QString>                    custom_name;
                const auto&                               color_style = colorMapStyle(tiny_type, custom_name);

                custom_colormap.m_color_values.clear();    /// 色は覚えない（オリジナルスタイルだけ覚える）
                custom_colormap.m_original_style = color_style;
                custom_colormap.m_original_tiny_style =
                    tiny_type.has_value() ? tiny_type.value() : tinycolormap::ColormapType(0);

                custom_colormap.m_range_min = ui->doubleSpinBox_result3d_color_range_min->value();
                custom_colormap.m_range_max = ui->doubleSpinBox_result3d_color_range_max->value();
            }

            QString original_name = color_item->text();
            if (original_name.lastIndexOf("*") == original_name.size() - 1) {
                original_name.removeLast();
            }

            SaveColorMapDlg dlg(ui->resultTab, ui, original_name);
            dlg.checkSaveMaxColor(custom_colormap.m_max_color.has_value());
            dlg.checkSaveMinColor(custom_colormap.m_min_color.has_value());
            dlg.checkSaveRange(custom_colormap.m_save_range_max.has_value());    /// minも同時
            dlg.checkSaveSteps(custom_colormap.m_save_steps.has_value());
            dlg.checkSaveGradient(custom_colormap.m_save_gradient.has_value());
            dlg.checkSaveReverse(custom_colormap.m_save_reverse.has_value());
            if (dlg.exec() == QDialog::Accepted) {
                QString file_name = dlg.filename();
                if (file_name.endsWith(".csv", Qt::CaseInsensitive)) {
                    file_name.chop(4);
                }

                QString original_style;
                if (custom_colormap.m_original_style == ColorMapStyle::E2ViewerLike) {
                    original_style = E2VIEWER_NAME;
                }
                else {
                    for (auto& info : m_colormap_default_list) {
                        if (custom_colormap.m_original_style == info.m_style) {
                            original_style = info.m_name;
                            break;
                        }
                    }
                }

                /// @Version,1
                /// @OriginalStyle,name
                /// @MaxColor,R,G,B
                /// @MinColor,R,G,B
                /// @Steps,number,
                /// @Gradient,
                /// @Range,min,max
                /// @Reverse,
                /// @CurRange,range_min,range_max
                /// @Color
                /// R,G,B
                /// R,G,B
                /// ...
                QList<QStringList> write_data;
                QStringList version, style, max_color, min_color, steps, gradient, range, reverse, currange, color;
                version << "@Version"
                        << "1";
                style << "@OriginalStyle" << (QString("\"") + original_style + QString("\""));
                max_color << "@MaxColor";
                if (dlg.isSaveMaxColor()) {
                    max_color << QString::number((int)(m_color_max.x() * 255.0f));
                    max_color << QString::number((int)(m_color_max.y() * 255.0f));
                    max_color << QString::number((int)(m_color_max.z() * 255.0f));

                    custom_colormap.m_max_color = m_color_max;
                }
                else {
                    custom_colormap.m_max_color.reset();
                }
                min_color << "@MinColor";
                if (dlg.isSaveMinColor()) {
                    min_color << QString::number((int)(m_color_min.x() * 255.0f));
                    min_color << QString::number((int)(m_color_min.y() * 255.0f));
                    min_color << QString::number((int)(m_color_min.z() * 255.0f));

                    custom_colormap.m_min_color = m_color_min;
                }
                else {
                    custom_colormap.m_min_color.reset();
                }
                steps << "@Steps";
                if (dlg.isSaveSteps()) {
                    steps << QString::number(ui->spinBox_result3d_color_count->value());
                    custom_colormap.m_save_steps = ui->spinBox_result3d_color_count->value();
                }
                else {
                    custom_colormap.m_save_steps.reset();
                }
                gradient << "@Gradient";
                if (dlg.isSaveGradient()) {
                    gradient << (ui->checkBox_result3d_color_gradient->isChecked() ? "1" : "0");
                    custom_colormap.m_save_gradient = ui->checkBox_result3d_color_gradient->isChecked();
                }
                else {
                    custom_colormap.m_save_gradient.reset();
                }
                range << "@Range";
                if (dlg.isSaveRange()) {
                    range << QString::number(ui->doubleSpinBox_result3d_color_range_min->value());
                    custom_colormap.m_save_range_min = ui->doubleSpinBox_result3d_color_range_min->value();
                    range << QString::number(ui->doubleSpinBox_result3d_color_range_max->value());
                    custom_colormap.m_save_range_max = ui->doubleSpinBox_result3d_color_range_max->value();
                }
                else {
                    custom_colormap.m_save_range_min.reset();
                    custom_colormap.m_save_range_max.reset();
                }
                reverse << "@Reverse";
                if (dlg.isSaveReverse()) {
                    reverse << (ui->checkBox_result3d_color_reverse->isChecked() ? "1" : "0");
                    custom_colormap.m_save_reverse = ui->checkBox_result3d_color_reverse->isChecked();
                }
                else {
                    custom_colormap.m_save_reverse.reset();
                }
                currange << "@CurRange" << QString::number(custom_colormap.m_range_min)
                         << QString::number(custom_colormap.m_range_max);

                write_data << version;
                write_data << style;
                write_data << max_color;
                write_data << min_color;
                write_data << steps;
                write_data << gradient;
                write_data << range;
                write_data << reverse;
                write_data << currange;

                color << "@Color";
                write_data << color;
                for (auto& color_value : custom_colormap.m_color_values) {
                    color.clear();
                    color << QString::number((int)(color_value.x() * 255.0f));
                    color << QString::number((int)(color_value.y() * 255.0f));
                    color << QString::number((int)(color_value.z() * 255.0f));
                    write_data << color;
                }

                SettingCtrl::createDirectoryIfNotExists(SettingCtrl::resultColorMapPath());

                CsvUtil::writeCsv(SettingCtrl::resultColorMapPath() + "/" + file_name + ".csv", write_data);

                if (is_custom == 2) {
                    ui->comboBox_result3d_custom_setting->blockSignals(true);
                    deleteColorStyle();
                    ui->comboBox_result3d_custom_setting->blockSignals(false);
                }
                appendCustomColorStyle(file_name, custom_colormap);
            }
        }
    }
}

void ResultCtrl::deleteColorStyle()
{
    int index = ui->comboBox_result3d_custom_setting->currentIndex();

    auto model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    if (index >= 0 && index < model->rowCount()) {
        auto color_item = model->item(index);
        if (color_item) {
            int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
            if (is_custom == 2) {
                m_custom_colormap.erase(color_item->text());

                if (index > 0) ui->comboBox_result3d_custom_setting->setCurrentIndex(index - 1);
                ui->comboBox_result3d_custom_setting->removeItem(index);
                // setColorBand(-1, -1, true); // setCurrentIndexで呼ばれる
            }
            else if (is_custom == 1) {
                QString name = color_item->text();
                if (name == E2VIEWER_NAME) {
                    return;
                }

                /// 保存済みに対する編集中があれば先にそれを削除
                auto color_item_edit = model->item(index + 1);
                if (color_item_edit) {
                    int is_custom =
                        color_item_edit->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
                    if (is_custom == 2) {
                        m_custom_colormap.erase(color_item_edit->text());
                        ui->comboBox_result3d_custom_setting->removeItem(index + 1);
                        // setColorBand(-1, -1, true); // setCurrentIndexで呼ばれる
                    }
                }

                m_custom_colormap.erase(name);

                if (index > 1) {
                    ui->comboBox_result3d_custom_setting->setCurrentIndex(index - 1);
                }
                else if (index == 1) {
                    if (index + 1 < model->rowCount()) {
                        auto next_item = model->item(index + 1);
                        if (next_item && next_item->data(Qt::UserRole + 2).toInt() == 1) {
                            ui->comboBox_result3d_custom_setting->setCurrentIndex(index + 1);
                        }
                        else {
                            ui->comboBox_result3d_custom_setting->setCurrentText(E2VIEWER_NAME);
                        }
                    }
                    else {
                        ui->comboBox_result3d_custom_setting->setCurrentText(E2VIEWER_NAME);
                    }
                }

                ui->comboBox_result3d_custom_setting->removeItem(index);
                // setColorBand(-1, -1, true);

                /// ファイル削除
                QFile file(SettingCtrl::resultColorMapPath() + "/" + name + ".csv");
                if (file.exists()) {
                    file.remove();
                }
            }
        }
    }
}

void ResultCtrl::setEnambleStyleSaveDelete()
{
    bool isSave   = false;
    bool isDelete = false;

    int index = ui->comboBox_result3d_custom_setting->currentIndex();

    auto model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    if (index >= 0 && index < model->rowCount()) {
        auto color_item = model->item(index);
        if (color_item) {
            int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
            if (is_custom == 2) {
                isSave   = true;
                isDelete = true;
            }
            else if (is_custom == 1) {
                if (color_item->text() != E2VIEWER_NAME) {
                    isSave   = true;
                    isDelete = true;
                }
                else {
                    isSave = true;    /// 常に保存可にする
                }
            }
            else {
                isSave = true;    /// 常に保存可にする
            }
        }
    }

    ui->pushButton_result3d_custom_setting_save->setEnabled(isSave);
    ui->pushButton_result3d_custom_setting_delete->setEnabled(isDelete);
}

void ResultCtrl::reset()
{
    ui->groupBox_result3d_specified_range->setChecked(false);
    ui->checkBox_result3d_show_only_in_voxel_material->setChecked(false);
    for (int row = 0; row < ui->tableWidget_result3d_range_list->rowCount(); ++row) {
        QCheckBox* check_box =
            (QCheckBox*)ui->tableWidget_result3d_range_list->cellWidget(row, 0)->findChild<QCheckBox*>();
        if (check_box) {
            check_box->blockSignals(true);
            check_box->setChecked(false);
            check_box->blockSignals(false);
        }
    }

    if (m_voxel_3d != nullptr) {
        ui->checkBox_result3d_view->blockSignals(true);
        ui->checkBox_result3d_view->setChecked(true);
        ui->checkBox_result3d_view->blockSignals(false);
        m_voxel_3d->show();
    }

    result2dListShow(true, true, nullptr, false);
}

void ResultCtrl::showOpt(bool show)
{
    setResult3dView(show);
}

void ResultCtrl::showOp2(bool show)
{
    result2dListShow(show, true);
}

void ResultCtrl::setVoxel(RefPtr<Node> voxel_node)
{
    if (m_voxel_3d != nullptr) {
        m_voxel_range_voxels.erase(m_voxel_3d);
    }
    m_voxel_3d = voxel_node;

    if (m_voxel_3d.isAlive()) {
        VoxelScalar* voxel_scalar = (VoxelScalar*)m_voxel_3d->object();

        ui->lineEdit_result3d_opt_max->setText(QString::number(voxel_scalar->maxValue(), 'E', 5));
        ui->lineEdit_result3d_opt_min->setText(QString::number(voxel_scalar->minValue(), 'E', 5));
        ui->pushButton_result3d_opt_detail->setEnabled(true);
        ui->pushButton_result3d_set_to_colormap->setEnabled(true);

        /// 読み込み時は無効にする (or 反映するならその処理がいる)
        /*
        ui->groupBox_result3d_specified_range->setChecked(false);
        ui->checkBox_result3d_show_only_in_voxel_material->setChecked(false);
        for (int row = 0; row < ui->tableWidget_result3d_range_list->rowCount(); ++row) {
            QCheckBox* check_box =
                (QCheckBox*)ui->tableWidget_result3d_range_list->cellWidget(row, 0)->findChild<QCheckBox*>();
            if (check_box) {
                check_box->blockSignals(true);
                check_box->setChecked(false);
                check_box->blockSignals(false);
            }
        }*/
        /// 反映する
        if (ui->groupBox_result3d_specified_range->isChecked()
            || ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
            createVoxelRangeShape();
        }
    }
    else {
        ui->lineEdit_result3d_opt_max->setText("");
        ui->lineEdit_result3d_opt_min->setText("");
        ui->pushButton_result3d_opt_detail->setEnabled(false);
        ui->pushButton_result3d_set_to_colormap->setEnabled(false);
    }

    setMinMaxLabel();
}

ResultCtrl::ColorMapStyle ResultCtrl::colorMapStyle(std::optional<tinycolormap::ColormapType>& tiny_type,
                                                    std::optional<QString>&                    style_name) const
{
    int index = ui->comboBox_result3d_custom_setting->currentIndex();

    auto model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    if (index >= 0 && index < model->rowCount()) {
        auto color_item = model->item(index);
        if (color_item) {
            int is_custom = color_item->data(Qt::UserRole + 2).toInt();    /// 0:標準,1:保存済,2:メモリ上編集
            if (is_custom != 0) {
                style_name = color_item->text();
                return ColorMapStyle::Custom;
            }
            else {
                int list_index = color_item->data(Qt::UserRole + 1).toInt();
                if (list_index >= 0 && list_index < m_colormap_default_list.size()) {
                    if (m_colormap_default_list[list_index].m_style == ColorMapStyle::Separator) {
                        /// 不正
                        assert(0);
                        return ColorMapStyle::Custom;
                    }
                    else {
                        tiny_type = m_colormap_default_list[list_index].m_tiny_type;
                        return m_colormap_default_list[list_index].m_style;
                    }
                }
            }
        }
    }

    return ColorMapStyle::Custom;
}

/// 両端含んだ全体の色数
int ResultCtrl::colorCount() const
{
    if (ui->checkBox_result3d_color_gradient->isChecked()) {
        /// 最大255(1byte)におさめるかつ、数値表記のずれ防止で調整（分割位置で割り切れるようにする）
        /// (colorLabelCount() - 2) * n + 2　＜ 250(最大数)となるn
        int n           = std::floor((250 - 2) / (colorLabelCount() - 2));
        int color_count = (colorLabelCount() - 2) * n + 2;
        // DEBUG() << color_count;
        return color_count;
    }
    return colorLabelCount();
}

/// カラーラベルの数（両端のエディットボックスも含む）
int ResultCtrl::colorLabelCount() const
{
    return ui->spinBox_result3d_color_count->value() + 2;
}

bool ResultCtrl::setColorBand(float input_min, float input_max, bool reset_color, bool by_file_open)
{
    if (m_suppress_set_color) {
        return false;
    }

    /// ファイルオープン時
    if (by_file_open) {
        if (ui->checkBox_result3d_apply_color_minmax->isChecked()) {
            /// 最大最小で更新
            if (m_result_all_min != FLT_MAX) {
                ui->LE_result3d_min_value->setText(QString::number(m_result_all_min, 'E', 5));
                ui->LE_result3d_max_value->setText(QString::number(m_result_all_max, 'E', 5));
            }
        }
        else {
            /// 妥当な数値が入っていなかったら更新する
            bool valid_value = false;
            if (!ui->LE_result3d_min_value->text().isEmpty() && !ui->LE_result3d_max_value->text().isEmpty()) {
                float max_value = ui->LE_result3d_max_value->text().toFloat();
                float min_value = ui->LE_result3d_min_value->text().toFloat();
                if (max_value > min_value) {
                    valid_value = true;
                }
            }
            if (!valid_value) {
                if (input_min >= 0) {
                    ui->LE_result3d_min_value->setText(QString::number(input_min, 'E', 5));
                    if (m_set_opt_op2_open) {
                        m_valid_color_map_min = input_min;
                    }
                }
                if (input_max >= 0) {
                    ui->LE_result3d_max_value->setText(QString::number(input_max, 'E', 5));
                    if (m_set_opt_op2_open) {
                        m_valid_color_map_max = input_max;
                    }
                }
                if (m_set_opt_op2_open) {
                    m_invalid_color_map_before_open = true;
                }
            }
            /// 暫定 - ややこしいがopt op2同時読込み時に関数2回呼ばれるのを考慮
            else if (m_set_opt_op2_open && m_invalid_color_map_before_open) {
                if (input_min >= 0) {
                    if ((m_valid_color_map_min >= 0) && (m_valid_color_map_min < input_min)) {
                    }
                    else {
                        ui->LE_result3d_min_value->setText(QString::number(input_min, 'E', 5));
                        m_valid_color_map_min = input_min;
                    }
                }
                if (input_max >= 0) {
                    if ((m_valid_color_map_max >= 0) && (m_valid_color_map_max > input_max)) {
                    }
                    else {
                        ui->LE_result3d_max_value->setText(QString::number(input_max, 'E', 5));
                        m_valid_color_map_max = input_max;
                    }
                }
            }
        }
    }
    else {
        if (input_min >= 0) {
            ui->LE_result3d_min_value->setText(QString::number(input_min, 'E', 5));
        }
        if (input_max >= 0) {
            ui->LE_result3d_max_value->setText(QString::number(input_max, 'E', 5));
        }
    }

    if (m_3DForm->utilityCtrl()) {
        m_3DForm->utilityCtrl()->update();
    }

    bool view_update = false;

    if (reset_color || colorCount() != m_color_values.size()) {
        setColorMap();
        view_update = true;
    }

    if (!ui->LE_result3d_min_value->text().isEmpty() && !ui->LE_result3d_max_value->text().isEmpty()) {
        float max_value = ui->LE_result3d_max_value->text().toFloat();
        float min_value = ui->LE_result3d_min_value->text().toFloat();

        /// ゼロ調整をGUI表記もする場合
        if (ui->rb_result3d_cmode_LOG->isChecked()) {
            if (min_value <= 0) {
                // min_value = max_value * 1e-10;
                // min_value = std::numeric_limits<float>::min();
                // ui->LE_result3d_min_value->setText(QString::number(min_value));
            }
        }

        if (by_file_open && ui->checkBox_result3d_display_in_view_auto->isChecked()) {
            ui->checkBox_result3d_display_in_view->blockSignals(true);
            ui->checkBox_result3d_display_in_view->setChecked(true);
            ui->checkBox_result3d_display_in_view->blockSignals(false);
            ui->obj3dViewer->setShowColorBar(true);
            view_update = true;
        }

        // if (max_value == min_value) {
        //     float next = std::nextafter(max_value, std::numeric_limits<float>::infinity());
        //     max_value  = next;
        // }

        if (max_value >= min_value) {
            m_threshold_label_values.clear();

            setThreshold(min_value, max_value, !ui->rb_result3d_cmode_LOG->isChecked());
            ui->obj3dViewer->set3DTextureColor(m_threshold_values, m_color_values);
            ui->obj3dViewer->setColorLabelCount(colorLabelCount());
            // if (m_voxel_3d.isAlive()) {
            //     ui->obj3dViewer->create3DTexture((VoxelScalar*)m_voxel_3d->object());
            // }
            float step       = 1;
            int   loop_count = m_threshold_values.size();

            if (colorLabelCount() != m_color_values.size()) {
                loop_count = colorLabelCount() - 1;
                step       = (float)(m_threshold_values.size() - 1) / (float)(loop_count - 1);
            }

            m_threshold_label_values << max_value;

            int label_count = 0;

            /// i==0とi==loop_count-1(両端)は通らないが、Renderのソースと合わせておく
            for (int i = 1; i < loop_count - 1; ++i) {
                int index = i;
                if (i != 0 && i != loop_count - 1) {
                    index = (int)((float)step * (float)(i) + 0.5);
                }
                else if (i == loop_count - 1) {
                    index = m_threshold_values.size() - 1;
                }

                m_threshold_labels[label_count]->setText(QString::number(m_threshold_values[index], 'E', 5));
                m_threshold_label_values << m_threshold_values[index];
                ++label_count;
            }

            m_threshold_label_values << min_value;

            view_update = true;
        }
    }

    if (view_update) {
        ui->obj3dViewer->update();
    }
    return true;
}

void ResultCtrl::calcThresholds(float min, float max, std::vector<float>& thresholds)
{
    bool linear = !ui->rb_result3d_cmode_LOG->isChecked();

    int color_count = colorCount();

    thresholds.clear();
    thresholds.resize(color_count - 1);

    if (linear) {
        float d = max - min;
        for (int ic = 0; ic < color_count - 1; ++ic) {
            thresholds[color_count - 2 - ic] = d * (1.0 / (double)(color_count - 2)) * (double)ic + min;
        }
    }
    else {
        /// こっちは入力制御していなければあり得る
        if (min <= 0) {
            min = std::numeric_limits<float>::min();
        }
        /// 不正防止
        if (max <= 0) {
            max = std::numeric_limits<float>::min();
        }
        double dl = log10(max) - log10(min);
        for (int ic = 0; ic < color_count - 1; ++ic) {
            thresholds[color_count - 2 - ic] =
                pow(10.0, log10(min) + dl * (1.0 / (double)(color_count - 2)) * (double)ic);
            /// ゼロにする
            if (ic == 0 && min == std::numeric_limits<float>::min()) {
                thresholds[color_count - 2 - ic] = 0;
            }
        }
    }
}

void ResultCtrl::addVoxel2d(std::vector<RefPtr<Node>>& voxel_nodes, std::map<QString, WeakRefPtr<Node>>& path_to_node)
{
    ui->tableWidget_result3d_op2_list->setUpdatesEnabled(false);

    ui->tableWidget_result3d_op2_list->setSortingEnabled(false);

    int org_row_count = ui->tableWidget_result3d_op2_list->rowCount();

    ui->tableWidget_result3d_op2_list->setRowCount(org_row_count + voxel_nodes.size());

    for (auto& voxel_node : voxel_nodes) {
        m_voxel_2d_list.emplace_back(voxel_node);
        bool d_section = setDSectionMatrix(voxel_node.ptr());
        add2dResultList(voxel_node.ptr(), d_section, org_row_count++);
    }

    ui->tableWidget_result3d_op2_list->setSortingEnabled(true);

    ui->tableWidget_result3d_op2_list->setUpdatesEnabled(true);

    for (auto& [path, node] : path_to_node) {
        m_path_to_2dvoxel[path] = node;
    }

    setMinMaxLabel();

    if (ui->groupBox_result3d_specified_range->isChecked()
        || ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
        createVoxelRangeShape();
    }
}

bool ResultCtrl::setDSectionParam(int max_x_array, int max_y_array, bool auto_set)
{
    if (auto_set) {
        ui->spinBox_result3d_array_x->setValue(std::max(max_x_array, ui->spinBox_result3d_array_x->value()));
        ui->spinBox_result3d_array_y->setValue(std::max(max_y_array, ui->spinBox_result3d_array_y->value()));
        return true;
    }

    if (ui->checkBox_op2_load_dsection_dialog_hide->isChecked()) {
        ui->spinBox_result3d_array_x->setValue(std::max(max_x_array, ui->spinBox_result3d_array_x->value()));
        ui->spinBox_result3d_array_y->setValue(std::max(max_y_array, ui->spinBox_result3d_array_y->value()));
        return true;
    }

    Op2ArraySettingDlg dlg(m_3DForm);
    dlg.showCheckHideFileOpen();

    dlg.setArrayX(std::max(max_x_array, ui->spinBox_result3d_array_x->value()));
    dlg.setArrayY(std::max(max_y_array, ui->spinBox_result3d_array_y->value()));
    dlg.setVoxX(ui->doubleSpinBox_result3d_voxel_size_x->value());
    dlg.setVoxY(ui->doubleSpinBox_result3d_voxel_size_y->value());

    if (dlg.exec() == QDialog::Accepted) {
        ui->spinBox_result3d_array_x->setValue(dlg.arrayX());
        ui->spinBox_result3d_array_y->setValue(dlg.arrayY());
        ui->doubleSpinBox_result3d_voxel_size_x->setValue(dlg.voxX());
        ui->doubleSpinBox_result3d_voxel_size_y->setValue(dlg.voxY());

        ui->checkBox_op2_load_dsection_dialog_hide->setChecked(dlg.isCheckHideFileOpen());
        return true;
    }
    return false;
}

void ResultCtrl::createVoxelRangeList(std::vector<std::tuple<float, float, int>>& range_list,
                                      std::vector<std::pair<float, float>>&       range_disp_list)
{
    auto text_to_float = [](const QString& text, bool& ok) {
        ok = false;
        if (text.compare("MIN", Qt::CaseInsensitive) == 0) {
            ok = true;
            return 0.0f;
        }
        if (text.compare("MAX", Qt::CaseInsensitive) == 0) {
            ok = true;
            return FLT_MAX;
        }

        float value = text.toFloat(&ok);
        if (ok) {
            ok = true;
            return value;
        }
        else {
            return 0.0f;
        }
    };

    if (ui->groupBox_result3d_specified_range->isChecked()) {
        for (int row = 0; row < ui->tableWidget_result3d_range_list->rowCount(); row++) {
            QLineEdit* min_line = (QLineEdit*)ui->tableWidget_result3d_range_list->cellWidget(row, 1);
            QLineEdit* max_line = (QLineEdit*)ui->tableWidget_result3d_range_list->cellWidget(row, 2);

            if (!min_line->text().isEmpty() && !max_line->text().isEmpty()) {
                bool max_ok = false;
                bool min_ok = false;

                float max_value = text_to_float(max_line->text(), max_ok);
                float min_value = text_to_float(min_line->text(), min_ok);
                if (max_ok && min_ok) {
                    if (max_value >= min_value) {
                        range_list.emplace_back(min_value, max_value, row);
                    }
                    else {
                        range_list.emplace_back(max_value, min_value, row);
                    }

                    QCheckBox* enable_check =
                        (QCheckBox*)ui->tableWidget_result3d_range_list->cellWidget(row, 0)->findChild<QCheckBox*>();
                    if (enable_check->isChecked()) {
                        auto& range_back = range_list.back();
                        range_disp_list.emplace_back(std::get<0>(range_back), std::get<1>(range_back));
                    }
                }
            }
        }
    }
    else if (ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
        range_list.emplace_back(0, FLT_MAX, -1);
        range_disp_list.emplace_back(0, FLT_MAX);
    }
}

void ResultCtrl::createVoxelRangeShape()
{
    ui->obj3dViewer->suppressRender(true);

    std::vector<std::tuple<float, float, int>> range_list;
    std::vector<std::pair<float, float>>       range_disp_list;
    createVoxelRangeList(range_list, range_disp_list);

    if (range_list.size() > 0) {
        std::vector<Node*> targets;
        if (m_voxel_3d.isAlive()) {
            targets.emplace_back(m_voxel_3d.ptr());
        }
        for (auto& node : m_voxel_2d_list) {
            if (node.isAlive()) {
                targets.emplace_back(node.ptr());
            }
        }

        createVoxelRangeShape(targets, range_list, range_disp_list);
    }
    else {
        /// 空で作る
        std::vector<Node*> targets;
        if (m_voxel_3d.isAlive()) {
            targets.emplace_back(m_voxel_3d.ptr());
        }
        for (auto& node : m_voxel_2d_list) {
            if (node.isAlive()) {
                targets.emplace_back(node.ptr());
            }
        }

        createVoxelRangeShape(targets, range_list, range_disp_list);
    }

    ui->obj3dViewer->setViewNearFar();
    ui->obj3dViewer->suppressRender(false);
}

bool CreateRangeVoxelScalar::is_in_merged_range(const std::vector<std::pair<float, float>>& merged_range, float x)
{
    // firstでlower_bound
    auto it = std::lower_bound(merged_range.begin(), merged_range.end(), x,
                               [](const std::pair<float, float>& range, float value) {
                                   return range.second < value;    // 範囲の終了値 < x なら次へ
                               });
    if (it == merged_range.end()) return false;
    // 範囲の開始値 <= x <= 範囲の終了値 ならOK
    return it->first <= x && x <= it->second;
}

void CreateRangeVoxelScalar::processVoxelRegion(int ix_min, int ix_max, int iy_min, int iy_max, int iz_min, int iz_max,
                                                int Nx, int Ny, int Nz, const float* data,
                                                const std::vector<std::pair<float, float>>& merged_range,
                                                Voxel*                                      voxel_scalar_obj)
{
    std::vector<int> iz_list(iz_max - iz_min + 1);
    std::iota(iz_list.begin(), iz_list.end(), iz_min);

    size_t NyNz = Ny * Nz;
    std::for_each(std::execution::par, iz_list.begin(), iz_list.end(), [&](int iz) {
        for (int iy = iy_min; iy <= iy_max; ++iy) {
            const float* p  = &data[iy * Nz + iz];
            int          ix = ix_min;
            while (ix <= ix_max) {
                const float* cur   = p + ix * NyNz;
                float        value = *cur;
                if (!is_in_merged_range(merged_range, value)) {
                    ++ix;
                    continue;
                }
                int ixcur = ix;
                while (ix + 1 <= ix_max) {
                    const float* next       = p + (ix + 1) * NyNz;
                    float        next_value = *next;
                    if (!is_in_merged_range(merged_range, next_value)) {
                        break;
                    }
                    ++ix;
                }
                if (ixcur == ix) {
                    voxel_scalar_obj->setCell(ix - ix_min, iy - iy_min, iz - iz_min, true);
                }
                else {
                    voxel_scalar_obj->setXCells(ixcur - ix_min, ix - ix_min, iy - iy_min, iz - iz_min, true);
                }
                ix += 2;
            }
        }
    });
}

void CreateRangeVoxelScalar::processVoxelRegion(int ix_min, int ix_max, int iy_min, int iy_max, int iz_min, int iz_max,
                                                int Nx, int Ny, int Nz, const float* data, float min_value,
                                                float max_value, Voxel* voxel_scalar_obj)
{
    std::vector<size_t> iz_list(iz_max - iz_min + 1);
    std::iota(iz_list.begin(), iz_list.end(), iz_min);

    size_t NyNz = Ny * Nz;
    std::for_each(std::execution::par, iz_list.begin(), iz_list.end(), [&](size_t iz) {
        for (size_t iy = iy_min; iy <= iy_max; ++iy) {
            const float* p  = &data[iy * Nz + iz];
            int          ix = ix_min;
            while (ix <= ix_max) {
                const float* cur   = p + ix * NyNz;
                float        value = *cur;
                if (value < min_value || value > max_value) {
                    ++ix;
                    continue;
                }
                int ixcur = ix;
                while (ix + 1 <= ix_max) {
                    const float* next       = p + (ix + 1) * NyNz;
                    float        next_value = *next;
                    if (next_value < min_value || next_value > max_value) {
                        break;
                    }
                    ++ix;
                }
                if (ixcur == ix) {
                    voxel_scalar_obj->setCell(ix - ix_min, iy - iy_min, iz - iz_min, true);
                }
                else {
                    voxel_scalar_obj->setXCells(ixcur - ix_min, ix - ix_min, iy - iy_min, iz - iz_min, true);
                }
                ix += 2;
            }
        }
    });
}

void CreateRangeVoxelScalar::processVoxelRegion(int ix_min, int ix_max, int iy_min, int iy_max, size_t Nx, size_t Ny,
                                                const float* data, float min_value, float max_value,
                                                Voxel* voxel_scalar_obj)
{
    std::vector<size_t> iy_list(iy_max - iy_min + 1);
    std::iota(iy_list.begin(), iy_list.end(), iy_min);

    /// setCellがZ同一の並列に対応していない。とりあえずseqにする
    std::for_each(std::execution::seq, iy_list.begin(), iy_list.end(), [&](size_t iy) {
        const float* p  = &data[iy * Nx];
        int          ix = ix_min;
        while (ix <= ix_max) {
            const float* cur   = p + ix;
            float        value = *cur;
            if (value < min_value || value > max_value) {
                ++ix;
                continue;
            }
            int ixcur = ix;
            while (ix + 1 <= ix_max) {
                const float* next       = p + (ix + 1);
                float        next_value = *next;
                if (next_value < min_value || next_value > max_value) {
                    break;
                }
                ++ix;
            }
            if (ixcur == ix) {
                voxel_scalar_obj->setCell(ix - ix_min, iy - iy_min, 0, true);
            }
            else {
                voxel_scalar_obj->setXCells(ixcur - ix_min, ix - ix_min, iy - iy_min, 0, true);
            }
            ix += 2;
        }
    });
}

void ResultCtrl::setProgMessage(int value)
{
    if (m_pd) {
        m_pd->setValue(value);
    }
}

/*
void ResultCtrl::createRenderData(RangeCalcData* region)
{
    if (m_pd) {
        if (m_pd->wasCanceled()) {
            region->m_create_voxel = nullptr;
            return;
        }
    }
    // region->m_create_voxel->setVboUse(false);
    ui->obj3dViewer->createRenderData(region->m_create_voxel.ptr());
}*/

bool ResultCtrl::createVoxelRangeShape(const std::vector<Node*>&                         voxel_scales,
                                       const std::vector<std::tuple<float, float, int>>& range_list,
                                       const std::vector<std::pair<float, float>>& range_disp_list, QWidget* pd_parent,
                                       bool gui_no_change)
{
    /// 範囲内の形状ボクセルを生成する
    ///
    /// ※ややこしくなってしまったが
    /// 　行単位で生成して、変更があったものだけ作成するようにした
    /// 　行全体で１個の方が高速だが、表示の切り替えとか考慮した場合、個別の方が速かったり使い勝手良さそうなので

    std::vector<std::tuple<Node*, float, float>> voxel_ranges_list;    /// In

    std::vector<RangeCalcData> region_params_list;    /// Out

    std::map<float, std::set<float>> exists_voxel_values;
    for (auto& range : range_list) {
        exists_voxel_values[std::get<0>(range)].insert(std::get<1>(range));
    }

    /// 計算対象の特定
    for (auto& voxel_scale : voxel_scales) {
        if (!voxel_scale) {
            continue;
        }
        auto voxel_scalar_obj = voxel_scale->object<VoxelScalar>();
        if (!voxel_scalar_obj) {
            continue;
        }
        const float* data = voxel_scalar_obj->scalarData();
        if (!data) {
            continue;
        }

        /// 一旦消す。キャンセルしても最後に保持しているインスタンスから復帰する
        voxel_scale->removeAllChild();

        auto& range_voxels = m_voxel_range_voxels[voxel_scale];

        /// 生成対象 or 既存データの取り出し
        ///  range_voxels[][]アクセスでnullで生成しておく(マルチでアクセスするので、先にmapは作っておく）
        std::vector<std::tuple<float, float, RefPtr<Node>>> range_voxel_list;
        std::map<float, std::set<float>>                    check_disp_values;
        for (auto& range : range_disp_list) {
            if (!check_disp_values[range.first].insert(range.second).second) {
                continue;
            }
            range_voxel_list.emplace_back(range.first, range.second, range_voxels[range.first][range.second]);
        }

        /// リストにない場合は削除
        std::erase_if(range_voxels, [&](auto& outer) {
            float x         = outer.first;
            auto& inner_map = outer.second;
            std::erase_if(inner_map, [&](auto& inner) {
                float y = inner.first;
                return exists_voxel_values.count(x) == 0 || exists_voxel_values.at(x).count(y) == 0;
            });
            return inner_map.empty();
        });

        /// データあり
        /* /// キャンセルで不一致になる。最後に実施
        if (merged_range_disp_list.size() > 0) {
            voxel_scalar_obj->setRangeMinMax(merged_range_disp_list);
        }
        else {
            /// ダミーで設定（形状の投影の時、投影対象なしにする意図）
            std::vector<std::pair<float, float>> dummy;
            dummy.emplace_back(-1.0f, -1.0f);
            voxel_scalar_obj->setRangeMinMax(dummy);
        }
        */

        /// 範囲指定
        for (auto& range : range_voxel_list) {
            float         min_value  = std::get<0>(range);
            float         max_value  = std::get<1>(range);
            RefPtr<Node>& voxel_node = std::get<2>(range);

            /// 以前作成済み
            if (voxel_node != nullptr) {
                continue;
            }

            voxel_ranges_list.emplace_back(voxel_scale, min_value, max_value);
        }
    }

    m_pd = nullptr;

    bool thread_end    = false;
    bool thread_cancel = false;
    if (voxel_ranges_list.size() > 0) {
        m_pd = new QProgressDialog(pd_parent ? pd_parent : m_3DForm);
        m_pd->setMinimumDuration(500);
        m_pd->setModal(true);
        Qt::WindowFlags flags = m_pd->windowFlags();
        flags &= ~Qt::WindowCloseButtonHint;
        m_pd->setWindowFlags(flags);
        m_pd->setLabelText("Create Range Data");
        // m_pd->open();
        m_pd->setFixedSize(m_pd->sizeHint());
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

        connect(m_pd, &QProgressDialog::canceled, [this, &thread_cancel]() {
            if (m_pd) {
                thread_cancel = true;
                m_pd->setLabelText("Cancel and create partially...");
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

        std::vector<char>* compress_data = nullptr;
        if (ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
            compress_data = &m_voxel_file_compress_data;
        }

        CreateRangeVoxelScalar create_thread(compress_data, m_voxel_file_compress_data_original_size, ui->obj3dViewer,
                                             voxel_ranges_list, region_params_list);
        connect(&create_thread, &CreateRangeVoxelScalar::setProgMessage, this, &ResultCtrl::setProgMessage,
                Qt::QueuedConnection);
        // connect(&create_thread, &CreateRangeVoxelScalar::createRenderData, this, &ResultCtrl::createRenderData,
        //         Qt::QueuedConnection);
        connect(&create_thread, &CreateRangeVoxelScalar::finished, this, [&thread_end] { thread_end = true; });
        create_thread.start();

        while (1) {
            qApp->processEvents(Vox3DForm::isModalVisible(m_pd) ? QEventLoop::AllEvents
                                                                : QEventLoop::ExcludeUserInputEvents);
            if (thread_end) {
                break;
            }
            if (thread_cancel) {
                create_thread.setCancel();
            }
        }
    }

    if (m_pd) {
        /// 以降キャンセルできないのでキャンセルボタンをグレーアウト
        QList<QPushButton*> buttons = m_pd->findChildren<QPushButton*>();
        for (auto btn : buttons) {
            if (!btn->text().isEmpty()) {
                btn->setEnabled(false);    // グレーアウト
                qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }
    }

    /// キャンセルデータ
    std::map<float, std::set<float>> cancel_range;
    for (auto& region_param : region_params_list) {
        if (region_param.m_create_voxel == nullptr) {
            cancel_range[region_param.m_min_value].insert(region_param.m_max_value);
        }
    }

    for (auto& region_param : region_params_list) {
        if (!region_param.m_create_voxel.ptr()) {
            continue;
        }
        if (cancel_range[region_param.m_min_value].count(region_param.m_max_value)) {
            region_param.m_create_voxel = nullptr;
            continue;
        }

        if (region_param.m_create_voxel->renderMesh()->displayIndices().size() == 0) {
            continue;
        }

        auto& range_voxels = m_voxel_range_voxels[region_param.m_voxel_node];

        auto& new_node = range_voxels[region_param.m_min_value][region_param.m_max_value];
        if (new_node.ptr() == nullptr) {
            new_node = Node::createNode();
        }
        RefPtr<Node> new_child = new_node->addChild();
        new_child->setObject(region_param.m_create_voxel.ptr());
        ui->obj3dViewer->createRenderData(new_child->renderable());
    }

    std::map<float, std::set<float>> check_disp_values;
    for (auto& range : range_disp_list) {
        check_disp_values[range.first].insert(range.second);
    }

    std::for_each(std::execution::seq, voxel_scales.begin(), voxel_scales.end(), [&](auto& voxel_scale) {
        auto& range_voxels = m_voxel_range_voxels[voxel_scale];

        /// 行の対応をとるので全体でループ
        for (int ic = 0; ic < range_list.size(); ++ic) {
            auto& range = range_list[ic];

            if (check_disp_values[std::get<0>(range)].count(std::get<1>(range)) == 0) {
                voxel_scale->addChild();    /// 空
                continue;
            }

            /// キャンセルされていたらチェック外す
            if (gui_no_change == false) {
                if (cancel_range[std::get<0>(range)].count(std::get<1>(range))) {
                    if (std::get<2>(range) >= 0) {
                        auto check_box =
                            (QCheckBox*)ui->tableWidget_result3d_range_list->cellWidget(std::get<2>(range), 0)
                                ->findChild<QCheckBox*>();
                        if (check_box->isChecked()) {
                            check_box->blockSignals(true);
                            check_box->setChecked(false);
                            check_box->blockSignals(false);
                        }
                    }
                    else {
                        /// 範囲指定なしで、形状内指定のみの場合
                        if (ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
                            ui->checkBox_result3d_show_only_in_voxel_material->blockSignals(true);
                            ui->checkBox_result3d_show_only_in_voxel_material->setChecked(false);
                            ui->checkBox_result3d_show_only_in_voxel_material->blockSignals(false);
                        }
                    }
                }
            }

            RefPtr<Node> ref_node = range_voxels[std::get<0>(range)][std::get<1>(range)];
            if (!ref_node.ptr()) {
                voxel_scale->addChild();    /// 空
                continue;
            }

            /// 表示データ生成
            for (auto& ref_child : ref_node->children()) {
                ui->obj3dViewer->createRenderData(ref_child->renderable());
            }

            /// 元のボクセルノードの子ノードとして管理（インスタンス構造をここで利用してみる）
            auto new_copy = ref_node->copyInstance();
            new_copy->show();
            voxel_scale->appendChild(new_copy.ptr());
        }

        auto renderable = voxel_scale->renderable();
        if (renderable) {
            voxel_scale->clearDisplayData();
            ui->obj3dViewer->createRenderData(renderable);
        }
    });

    /// 投影用の範囲設定
    /// シェーダー用に最小最大範囲をマージ（最適化）
    std::vector<std::pair<float, float>> sorted_disp_list;
    for (auto& range : range_disp_list) {
        /// キャンセル除外
        if (cancel_range[std::get<0>(range)].count(std::get<1>(range))) {
            continue;
        }
        sorted_disp_list.emplace_back(range);
    }
    std::sort(sorted_disp_list.begin(), sorted_disp_list.end());
    std::vector<std::pair<float, float>> merged_range_disp_list;
    for (const auto& range : sorted_disp_list) {
        if (merged_range_disp_list.empty() || merged_range_disp_list.back().second < range.first) {
            merged_range_disp_list.push_back(range);
        }
        else {
            merged_range_disp_list.back().second = std::max(merged_range_disp_list.back().second, range.second);
        }
    }
    for (auto& voxel_scale : voxel_scales) {
        if (!voxel_scale) {
            continue;
        }
        auto voxel_scalar_obj = voxel_scale->object<VoxelScalar>();
        if (!voxel_scalar_obj) {
            continue;
        }
        const float* data = voxel_scalar_obj->scalarData();
        if (!data) {
            continue;
        }

        if (merged_range_disp_list.size() > 0) {
            voxel_scalar_obj->setRangeMinMax(merged_range_disp_list);
        }
        else {
            /// ダミーで設定（形状の投影の時、投影対象なしにする意図）
            std::vector<std::pair<float, float>> dummy;
            dummy.emplace_back(-1.0f, -1.0f);
            voxel_scalar_obj->setRangeMinMax(dummy);
        }
    }

    /// クリッピング更新
    m_3DForm->clippingCtrl()->updateClipping(true);

    ui->obj3dViewer->update();

    if (m_pd) {
        m_pd->hide();
        delete m_pd;
    }
    m_pd = nullptr;

    return !thread_cancel;
}

bool ResultCtrl::isEnableSpecifiedRange()
{
    if (ui->groupBox_result3d_specified_range->isChecked()
        || ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
        return true;
    }
    return false;
}

void CreateRangeVoxelScalar::run()
{
    /// このループではキャンセルさせない（m_targets生成しておかないと不整合になるので）
    for (auto& range : *m_input) {
        Node*        voxel_node       = std::get<0>(range);
        VoxelScalar* voxel_scalar_obj = voxel_node->object<VoxelScalar>();
        float        min_value        = std::get<1>(range);
        float        max_value        = std::get<2>(range);

        /// 作成
        Point3i min_index, max_index;
        voxel_scalar_obj->rangeIndex(min_value, max_value, min_index, max_index);

        int range_x = max_index.x() - min_index.x() + 1;
        int range_y = max_index.y() - min_index.y() + 1;
        int range_z = max_index.z() - min_index.z() + 1;

        /// 分割するならcreateDisplayDataの最小最大範囲指定が必要
        int Ndiv = 3;    // 3分割なら3、4分割なら4
        if (voxel_scalar_obj->is2DTexture()) {
            Ndiv = 1;
        }

        int index = 0;

        if (range_x == 1 && range_y == 1 && range_z == 1) {
            m_targets->emplace_back(voxel_node, min_value, max_value, min_index.x(), max_index.x(), min_index.y(),
                                    max_index.y(), min_index.z(), max_index.z(), index++);
        }
        else {
            int Ndiv_x = std::min(Ndiv, range_x);
            int Ndiv_y = std::min(Ndiv, range_y);
            int Ndiv_z = std::min(Ndiv, range_z);

            for (int ix_part = 0; ix_part < Ndiv_x; ++ix_part) {
                int ix_min = min_index.x() + (ix_part * range_x) / Ndiv_x;
                int ix_max = min_index.x() + ((ix_part + 1) * range_x) / Ndiv_x - 1;
                for (int iy_part = 0; iy_part < Ndiv_y; ++iy_part) {
                    int iy_min = min_index.y() + (iy_part * range_y) / Ndiv_y;
                    int iy_max = min_index.y() + ((iy_part + 1) * range_y) / Ndiv_y - 1;
                    if (voxel_scalar_obj->is2DTexture()) {
                        if (ix_min > ix_max || iy_min > iy_max) continue;
                        m_targets->emplace_back(voxel_node, min_value, max_value, ix_min, ix_max, iy_min, iy_max, 0, 0,
                                                index++);
                    }
                    else {
                        for (int iz_part = 0; iz_part < Ndiv_z; ++iz_part) {
                            int iz_min = min_index.z() + (iz_part * range_z) / Ndiv_z;
                            int iz_max = min_index.z() + ((iz_part + 1) * range_z) / Ndiv_z - 1;
                            if (ix_min > ix_max || iy_min > iy_max || iz_min > iz_max) continue;
                            m_targets->emplace_back(voxel_node, min_value, max_value, ix_min, ix_max, iy_min, iy_max,
                                                    iz_min, iz_max, index++);
                        }
                    }
                }
            }
        }
    }

    if (m_cancel) {
        return;
    }

    emit this->setProgMessage(5);

    /// ボクセルに投影する場合表示データをみる
    /// 現状読み込み時に全部同一のorg/delta/numberなのでこれで
    /// 異なるなら考慮した処理作る必要ある
    RefPtr<Voxel> voxel_disp_area;
    if (m_voxel_compress_data && m_compress_data_original_size > 0) {
        std::set<int> show_materials;
        Point3d       origin;
        Point3d       delta;
        Point3i       number;
        m_gl_widget->materialIds(show_materials, true);
        m_gl_widget->voxelCellInfo(origin, delta, number);

        voxel_disp_area = Voxel::createObject();
        voxel_disp_area->init(origin, delta, number);

        /// 表示あり
        if (show_materials.size() > 0) {
            /// 解凍
            std::vector<int> material_id_xs(m_compress_data_original_size);

            /// 解凍後のバイト数
            int dst_size = static_cast<int>(m_compress_data_original_size * sizeof(int));

            /// 解凍処理
            int decompressed_size =
                LZ4_decompress_safe(m_voxel_compress_data->data(),                      // 圧縮データの先頭
                                    reinterpret_cast<char*>(material_id_xs.data()),     // 出力先（int配列）
                                    static_cast<int>(m_voxel_compress_data->size()),    // 圧縮データサイズ
                                    dst_size                                            // 出力バッファサイズ
                );

            if (decompressed_size > 0) {
                /// データ読込み
                int current_x = 0;
                int current_y = 0;
                int current_z = 0;

                int number_x = number.x();
                int number_y = number.y();

                auto material_id_xs_size = material_id_xs.size();
                DEBUG() << material_id_xs_size;
                for (int ic = 0; ic < material_id_xs_size; ic += 2) {
                    auto material_id = material_id_xs[ic];
                    auto x_count     = material_id_xs[ic + 1];

                    /// 対象なら全体で設定。それ以外はインデックスの計算
                    Voxel* voxel_data = nullptr;
                    if (show_materials.count(material_id)) {
                        voxel_data = voxel_disp_area.ptr();
                    }

                    /// 範囲指定して設定
                    /// X最大(または超える)の場合（※超える場合考慮してwhile)
                    while (current_x + x_count >= number_x) {
                        if (voxel_data) {
                            voxel_data->setXCells(current_x, number_x - 1, current_y, current_z, true);
                        }

                        x_count -= (number_x - current_x);
                        current_x = 0;
                        ++current_y;
                        if (current_y == number_y) {
                            current_y = 0;
                            ++current_z;
                        }
                    }

                    if (x_count > 0) {
                        if (voxel_data) {
                            voxel_data->setXCells(current_x, current_x + x_count - 1, current_y, current_z, true);
                        }
                        current_x += x_count;
                    }
                }
            }
        }
    }

    emit this->setProgMessage(10);

    int prog_start_value = 10;
    int prog_end_value   = 90;

    double step =
        m_targets->size() > 0 ? ((double)prog_end_value - (double)prog_start_value) / (double)m_targets->size() : 1;
    int count     = 0;
    int pre_count = 0;
    int prog_step = 10;

    MutexWrapper mutex;

    float eps = m_gl_widget->sceneView()->sceneGraph()->lengthUnit().epsilonValidLength();

    std::vector<VoxelScalar*> back_voxels;
    if (voxel_disp_area != nullptr) {
        std::set<VoxelScalar*> voxels;
        for (auto& target : *m_targets) {
            Node* voxel_scale = target.m_voxel_node;
            auto  voxel_obj   = voxel_scale->object<VoxelScalar>();

            if (voxels.insert(voxel_obj).second) {
                if (voxel_obj->is2DTexture()) {
                    if (voxel_obj->setInvalid2dData(voxel_disp_area.ptr(), voxel_scale->pathMatrix(), eps)) {
                        back_voxels.emplace_back(voxel_obj);
                    }
                }
                else {
                    if (voxel_obj->setInvalidData(voxel_disp_area.ptr())) {
                        back_voxels.emplace_back(voxel_obj);
                    }
                }
            }
        }
    }

    /// マルチ計算
    std::for_each(
        std::execution::par, m_targets->begin(), m_targets->end(), [&](ResultCtrl::RangeCalcData& region_param) {
            if (m_cancel) {
                return;
            }
            Node* voxel_scale = region_param.m_voxel_node;
            float min_value   = region_param.m_min_value;
            float max_value   = region_param.m_max_value;
            int*  minmax      = region_param.m_minmax;

            auto voxel_scalar_obj = voxel_scale->object<VoxelScalar>();

            Point3d min_pos = voxel_scalar_obj->point(minmax[0], minmax[2], minmax[4]);

            auto voxel_scalar_child = VoxelScalar::createObject();
            voxel_scalar_child->init(min_pos.x(), min_pos.y(), min_pos.z(), voxel_scalar_obj->dX(),
                                     voxel_scalar_obj->dY(), voxel_scalar_obj->dZ(), minmax[1] - minmax[0] + 1,
                                     minmax[3] - minmax[2] + 1, minmax[5] - minmax[4] + 1);
            voxel_scalar_child->setCreateSectionLine(false);    /// 3D結果は輪郭作らない
            if (voxel_scalar_obj->is2DTexture()) {
                processVoxelRegion(minmax[0], minmax[1], minmax[2], minmax[3], voxel_scalar_obj->nX(),
                                   voxel_scalar_obj->nY(), voxel_scalar_obj->scalarData(), min_value, max_value,
                                   voxel_scalar_child.ptr());
                if (m_cancel) {
                    return;
                }

                voxel_scalar_child->set2DTexture(true);
                voxel_scalar_child->setBoundaryDeltaX(voxel_scalar_obj->boundaryDeltaX());
                voxel_scalar_child->setBoundaryOrgX(voxel_scalar_obj->boundaryOrgX());
                int offset[2] = {minmax[0], minmax[2]};
                voxel_scalar_child->createDisplayDataXYOnlyBoundary(voxel_scalar_obj->boundaryVoxel(), offset);
            }
            else {
                processVoxelRegion(minmax[0], minmax[1], minmax[2], minmax[3], minmax[4], minmax[5],
                                   voxel_scalar_obj->nX(), voxel_scalar_obj->nY(), voxel_scalar_obj->nZ(),
                                   voxel_scalar_obj->scalarData(), min_value, max_value, voxel_scalar_child.ptr());
                if (m_cancel) {
                    return;
                }
                voxel_scalar_child->createDisplayData();
            }

            voxel_scalar_child->deleteVoxelData();

            if (m_cancel) {
                return;
            }

            voxel_scalar_child->setProjectionOpt(voxel_scale);
            // voxel_scalar_child->setVboUse(false);
            region_param.m_create_voxel = voxel_scalar_child;

            mutex.lock();
            int cur_count = (int)(((double)count++ * step) + prog_start_value);
            if (cur_count > 100) cur_count = 100;
            if (cur_count / prog_step != pre_count / prog_step) {
                emit this->setProgMessage(cur_count / prog_step * prog_step);
                pre_count = cur_count;
            }

            // emit this->createRenderData(&region_param);
            mutex.unlock();
        });

    for (auto voxel : back_voxels) {
        voxel->backInvalidData();
    }

    if (!m_cancel) {
        emit this->setProgMessage(90);
    }
}

void ResultCtrl::clearVoxelRangeShape()
{
    if (m_voxel_3d.isAlive()) {
        m_voxel_3d->removeAllChild();
        auto voxel_scalar_obj = m_voxel_3d->object<VoxelScalar>();
        if (voxel_scalar_obj) {
            voxel_scalar_obj->setRangeMinMax(std::vector<std::pair<float, float>>());

            m_voxel_3d->clearDisplayData();
            voxel_scalar_obj->createShapeForTexture();

            ui->obj3dViewer->createRenderData(m_voxel_3d->renderable());
        }
    }

    for (auto& node : m_voxel_2d_list) {
        if (node.isAlive()) {
            node->removeAllChild();
            auto voxel_scalar_obj = node->object<VoxelScalar>();
            if (voxel_scalar_obj) {
                voxel_scalar_obj->setRangeMinMax(std::vector<std::pair<float, float>>());

                node->clearDisplayData();
                voxel_scalar_obj->createShapeForTexture();

                ui->obj3dViewer->createRenderData(node->renderable());
            }
        }
    }

    ui->obj3dViewer->setViewNearFar();

    /// クリッピング更新
    m_3DForm->clippingCtrl()->updateClipping(true);

    ui->obj3dViewer->update();
}

void ResultCtrl::create3dResultRangeList()
{
    QStringList labels;
    labels << "View"
           << "Min"
           << "Max";
    ui->tableWidget_result3d_range_list->setColumnCount(labels.size());
    ui->tableWidget_result3d_range_list->setHorizontalHeaderLabels(labels);
    ui->tableWidget_result3d_range_list->setStyleSheet("QTableWidget::item { padding: 0px; }");
    ui->tableWidget_result3d_range_list->verticalHeader()->setDefaultSectionSize(20);

    QVector<int> columnMargins = {15, 100, 100};    // 各列に対する余白の設定

    QFontMetrics font_metrics(ui->tableWidget_result3d_range_list->horizontalHeader()->font());
    for (int i = 0; i < labels.size(); ++i) {
        int size = font_metrics.horizontalAdvance(labels[i]);

        ui->tableWidget_result3d_range_list->setColumnWidth(i, size + columnMargins[i]);
    }

    ui->tableWidget_result3d_range_list->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(
        ui->tableWidget_result3d_range_list, &QTableWidget::customContextMenuRequested, [this](const QPoint& pos) {
            result3dRangeListContextMenu(ui->tableWidget_result3d_range_list->viewport()->mapToGlobal(pos));
        });

    ui->tableWidget_result3d_range_list->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->tableWidget_result3d_range_list->verticalHeader(), &QHeaderView::customContextMenuRequested,
                     this, [this](const QPoint& pos) {
                         result3dRangeListContextMenu(
                             ui->tableWidget_result3d_range_list->verticalHeader()->viewport()->mapToGlobal(pos));
                     });

    connect(ui->groupBox_result3d_specified_range, &QGroupBox::toggled, this, [this]() {
        if (ui->groupBox_result3d_specified_range->isChecked()
            || ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
            createVoxelRangeShape();
        }
        else {
            clearVoxelRangeShape();
            ui->tableWidget_result3d_range_list->clearSelection();
        }
    });
    connect(ui->checkBox_result3d_show_only_in_voxel_material, &QCheckBox::toggled, this, [this]() {
        if (ui->groupBox_result3d_specified_range->isChecked()
            || ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
            m_voxel_range_voxels.clear();
            createVoxelRangeShape();
        }
        else {
            clearVoxelRangeShape();
            ui->tableWidget_result3d_range_list->clearSelection();
        }
    });

    connect(ui->pushButton_result3d_opt_range_set_from_colormap, &QPushButton::clicked, this,
            [this]() { setResultRangeListFromColormap(); });

    connect(ui->pushButton_result3d_opt_range_add, &QPushButton::clicked, this, [this]() { add3dResultRangeList(); });
    connect(ui->pushButton_result3d_opt_range_insert, &QPushButton::clicked, this,
            [this]() { insert3dResultRangeList(); });
    connect(ui->pushButton_result3d_opt_range_delete, &QPushButton::clicked, this,
            [this]() { delete3dResultRangeList(false); });

    connect(ui->pushButton_result3d_opt_range_clear, &QPushButton::clicked, this, [this]() { clearResultRangeList(); });

    connect(ui->pushButton_result3d_opt_range_all_on, &QPushButton::clicked, this,
            [this]() { resultRangeListShow(true, true); });
    connect(ui->pushButton_result3d_opt_range_all_off, &QPushButton::clicked, this,
            [this]() { resultRangeListShow(false, true); });

    ui->tableWidget_result3d_range_list->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void ResultCtrl::result3dRangeListContextMenu(const QPoint& mouse_pos)
{
    /// 現在の選択を保持
    QItemSelectionModel* selection_model = ui->tableWidget_result3d_range_list->selectionModel();
    QModelIndexList      selectedIndexes = selection_model->selectedRows();
    QList<int>           rows;
    for (const QModelIndex& index : selectedIndexes) {
        rows << index.row();
    }
    std::sort(rows.begin(), rows.end());

    /// メニュー作成
    QMenu    contextMenu;
    QAction* action_show = contextMenu.addAction("On");
    QAction* action_hide = contextMenu.addAction("Off");
    contextMenu.addSeparator();
    QAction* action_del = contextMenu.addAction("Delete");

    /// 実行
    QAction* selectedAction = contextMenu.exec(mouse_pos);

    /// 選択を再現
    for (auto& row : rows) {
        QModelIndex index = ui->tableWidget_result3d_range_list->model()->index(row, 0);
        selection_model->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    if (selectedAction == action_show) {
        resultRangeListShow(true, false);
    }
    else if (selectedAction == action_hide) {
        resultRangeListShow(false, false);
    }
    else if (selectedAction == action_del) {
        /// 行削除
        delete3dResultRangeList(false);
    }
}

void ResultCtrl::updateResult()
{
    if (m_suppress_update_result) {
        return;
    }

    // if (ui->groupBox_result3d_specified_range->isChecked()) {
    if (ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
        m_voxel_range_voxels.clear();
        createVoxelRangeShape();
    }
    //}
}

void ResultCtrl::suppressUpdateResult(bool suppress)
{
    m_suppress_update_result = suppress;
    if (!m_suppress_update_result) {
        updateResult();
    }
}

void ResultCtrl::setVoxSize()
{
    BoundingBox3f vox_box;
    const auto&   vox_materials = ui->obj3dViewer->voxMaterials();
    for (const auto& [id, node] : vox_materials) {
        if (node.isAlive()) {
            Node* node_ptr = (Node*)node.ptr();    /// 暫定
            vox_box.expandBy(node_ptr->shapeBoundingBox());
        }
    }

    if (m_voxel_3d.isAlive()) {
        vox_box.expandBy(m_voxel_3d->shapeBoundingBox());
    }

    if (vox_box.valid()) {
        ui->doubleSpinBox_result3d_voxel_size_x->setValue(vox_box.getXLength() * 1e3f);
        ui->doubleSpinBox_result3d_voxel_size_y->setValue(vox_box.getYLength() * 1e3f);
    }
    else {
    }
}

void ResultCtrl::add3dResultRangeList(int row, const QString& min_value, const QString& max_value)
{
    auto table_widget = ui->tableWidget_result3d_range_list;
    if (table_widget->rowCount() == 30) {
        return;
    }

    if (row < 0) {
        row = table_widget->rowCount();
    }
    table_widget->insertRow(row);

    int column = 0;

    /// Enable
    QWidget*     checkBoxWidget = new QWidget();
    QCheckBox*   checkBox       = new QCheckBox(checkBoxWidget);
    QHBoxLayout* layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);
    checkBox->setChecked(false);

    connect(checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            createVoxelRangeShape();
        }
        else {
            createVoxelRangeShape();
        }
    });

    /// Min
    QLineEdit* line_edit = new QLineEdit();
    line_edit->setFrame(false);
    line_edit->setStyleSheet("QLineEdit { background: transparent; border: none; }");
    line_edit->setAlignment(Qt::AlignLeft);
    line_edit->setText(min_value);
    line_edit->setProperty("prev_text", min_value);
    line_edit->setContextMenuPolicy(Qt::NoContextMenu);
    table_widget->setCellWidget(row, column++, line_edit);
    connect(line_edit, &QLineEdit::editingFinished, this, [this, line_edit, checkBox]() {
        QString prev = line_edit->property("prev_text").toString();
        QString curr = line_edit->text();
        if (curr != prev) {
            line_edit->setProperty("prev_text", curr);    // 前回値更新
            if (checkBox->isChecked()) {
                createVoxelRangeShape();
            }
        }
    });

    /// Max
    line_edit = new QLineEdit();
    line_edit->setFrame(false);
    line_edit->setStyleSheet("QLineEdit { background: transparent; border: none; }");
    line_edit->setAlignment(Qt::AlignLeft);
    line_edit->setText(max_value);
    line_edit->setProperty("prev_text", max_value);
    line_edit->setContextMenuPolicy(Qt::NoContextMenu);
    table_widget->setCellWidget(row, column++, line_edit);
    connect(line_edit, &QLineEdit::editingFinished, this, [this, line_edit, checkBox]() {
        QString prev = line_edit->property("prev_text").toString();
        QString curr = line_edit->text();
        if (curr != prev) {
            line_edit->setProperty("prev_text", curr);    // 前回値更新
            if (checkBox->isChecked()) {
                createVoxelRangeShape();
            }
        }
    });
}

void ResultCtrl::insert3dResultRangeList()
{
    /// 削除の処理のIndexの取り方に合わせておく
    QItemSelectionModel* selection_model = ui->tableWidget_result3d_range_list->selectionModel();
    QModelIndexList      selectedIndexes = selection_model->selectedRows();
    QList<int>           rows;
    for (const QModelIndex& index : selectedIndexes) {
        rows << index.row();
    }
    std::sort(rows.begin(), rows.end());
    for (int row : rows) {
        add3dResultRangeList(row);
        return;
    }
}

void ResultCtrl::delete3dResultRangeList(bool all)
{
    for (int row = 0; row < ui->tableWidget_result3d_range_list->rowCount(); ++row) {
        QLineEdit* min_line = (QLineEdit*)ui->tableWidget_result3d_range_list->cellWidget(row, 1);
        QLineEdit* max_line = (QLineEdit*)ui->tableWidget_result3d_range_list->cellWidget(row, 2);
        min_line->blockSignals(true);
        max_line->blockSignals(true);
    }

    if (all) {
        ui->tableWidget_result3d_range_list->setRowCount(0);
    }
    else {
        QItemSelectionModel* selection_model = ui->tableWidget_result3d_range_list->selectionModel();
        QModelIndexList      selectedIndexes = selection_model->selectedRows();
        QList<int>           rows;
        for (const QModelIndex& index : selectedIndexes) {
            rows << index.row();
        }
        std::sort(rows.begin(), rows.end(), std::greater<int>());
        for (int row : rows) {
            ui->tableWidget_result3d_range_list->removeRow(row);
        }
        /* /// LineEditの埋め込みのせい？これが正しく機能しない。他も直した方がいいか
        QItemSelectionModel* selection_model = ui->tableWidget_result3d_range_list->selectionModel();
        for (int row = ui->tableWidget_result3d_range_list->rowCount() - 1; row >= 0; --row) {
            if (selection_model->isRowSelected(row)) {
                ui->tableWidget_result3d_range_list->removeRow(row);
            }
        }
        */
    }

    for (int row = 0; row < ui->tableWidget_result3d_range_list->rowCount(); ++row) {
        QLineEdit* min_line = (QLineEdit*)ui->tableWidget_result3d_range_list->cellWidget(row, 1);
        QLineEdit* max_line = (QLineEdit*)ui->tableWidget_result3d_range_list->cellWidget(row, 2);
        min_line->blockSignals(false);
        max_line->blockSignals(false);
    }

    createVoxelRangeShape();
}

void ResultCtrl::resultRangeListShow(bool show, bool all)
{
    std::vector<QCheckBox*> buttons;

    if (all) {
        for (int row = 0; row < ui->tableWidget_result3d_range_list->rowCount(); ++row) {
            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_result3d_range_list->cellWidget(row, 0)->findChild<QCheckBox*>());
        }
    }
    else {
        QItemSelectionModel* selection_model = ui->tableWidget_result3d_range_list->selectionModel();
        QModelIndexList      selectedIndexes = selection_model->selectedRows();
        QList<int>           rows;
        for (const QModelIndex& index : selectedIndexes) {
            rows << index.row();
        }
        std::sort(rows.begin(), rows.end());
        for (int row : rows) {
            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_result3d_range_list->cellWidget(row, 0)->findChild<QCheckBox*>());
        }
    }

    for (auto& button : buttons) {
        button->blockSignals(true);
        button->setChecked(show);
        button->blockSignals(false);
    }
    /// 一律ここで処理
    createVoxelRangeShape();
    ui->obj3dViewer->update();
}

void ResultCtrl::setResultRangeListFromColormap()
{
    if (m_threshold_label_values.size() == 0) {
        return;
    }

    clearVoxelRangeShape();
    delete3dResultRangeList(true);

    add3dResultRangeList(-1, QString::number(m_threshold_label_values[0], 'E', 5), "MAX");

    for (int ic = 1; ic < m_threshold_label_values.size(); ++ic) {
        add3dResultRangeList(-1, QString::number(m_threshold_label_values[ic], 'E', 5),
                             QString::number(m_threshold_label_values[ic - 1], 'E', 5));
    }

    add3dResultRangeList(-1, "MIN",
                         QString::number(m_threshold_label_values[m_threshold_label_values.size() - 1], 'E', 5));
}

void ResultCtrl::clearResultRangeList()
{
    delete3dResultRangeList(true);
    add3dResultRangeList();    /// デフォルト空行
}

void ResultCtrl::create2dReultTab()
{
    QStringList labels;
    labels << "View"
           << "Name"
           << "2D"
           << "3D"
           << "ToCM"
           << "Min"
           << "Max"
           << "Info"
           << "Arr X"
           << "Arr Y"
           << "Vox X"
           << "Vox Y"
           << "Folder";

    ui->tableWidget_result3d_op2_list->setColumnCount(labels.size());
    ui->tableWidget_result3d_op2_list->setHorizontalHeaderLabels(labels);
    ui->tableWidget_result3d_op2_list->setStyleSheet("QTableWidget::item { padding: 0px; }");
    ui->tableWidget_result3d_op2_list->verticalHeader()->setDefaultSectionSize(20);

    QVector<int> columnMargins = {10, 180, 15, 15, 15, 60, 60, 15, 16, 16, 22, 22, 180};    // 各列に対する余白の設定

    QFontMetrics font_metrics(ui->tableWidget_result3d_op2_list->horizontalHeader()->font());
    for (int i = 0; i < labels.size(); ++i) {
        int size = font_metrics.horizontalAdvance(labels[i]);

        ui->tableWidget_result3d_op2_list->setColumnWidth(i, size + columnMargins[i]);
    }

    ui->tableWidget_result3d_op2_list->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(ui->tableWidget_result3d_op2_list, &QTableWidget::customContextMenuRequested,
                     [this](const QPoint& pos) {
                         result2dListContextMenu(ui->tableWidget_result3d_op2_list->viewport()->mapToGlobal(pos));
                     });

    connect(ui->pushButton_result3d_op2_all_on, &QPushButton::clicked, this,
            [this]() { result2dListShow(true, true); });
    connect(ui->pushButton_result3d_op2_all_off, &QPushButton::clicked, this,
            [this]() { result2dListShow(false, true); });
    connect(ui->pushButton_result3d_op2_delete, &QPushButton::clicked, this, [this]() { deleteResult2dList(false); });
    connect(ui->pushButton_result3d_op2_all_clear, &QPushButton::clicked, this, [this]() { deleteResult2dList(true); });

    connect(ui->pushButton_result3d_op2_array_all_apply, &QPushButton::clicked, this,
            [this]() { applyOp2Array(true, false); });

    ui->tableWidget_result3d_op2_list->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->pushButton_result3d_import_op2, &QPushButton::clicked, this, [this]() {
        QStringList fileNames = QFileDialog::getOpenFileNames(m_3DForm, tr("Select file"), "", tr("op2 file (*.op2)"));
        m_3DForm->allLoad(fileNames);
    });
    connect(ui->pushButton_result3d_open_op2, &QPushButton::clicked, this, [this]() {
        QStringList fileNames = QFileDialog::getOpenFileNames(m_3DForm, tr("Select file"), "", tr("op2 file (*.op2)"));
        m_3DForm->allLoad(fileNames, false);
    });

    ui->tableWidget_result3d_op2_list->setItemDelegate(new NoElideDelegate(ui->tableWidget_result3d_op2_list));

    connect(ui->tableWidget_result3d_op2_list, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        if (column >= 2) {
            return;
        }

        Node* voxel_2d_scalar =
            (Node*)ui->tableWidget_result3d_op2_list->item(row, 1)->data(Qt::UserRole).toULongLong();

        QCheckBox* check_box =
            (QCheckBox*)ui->tableWidget_result3d_op2_list->cellWidget(row, 0)->findChild<QCheckBox*>();
        if (!check_box->isChecked()) {
            check_box->blockSignals(true);
            check_box->setChecked(true);
            ui->tableWidget_result3d_op2_list->setSortingEnabled(false);
            ui->tableWidget_result3d_op2_list->item(row, 0)->setText("");
            voxel_2d_scalar->show();
            check_box->blockSignals(false);
        }

        result2dListShow(false, true, voxel_2d_scalar, false);

        m_3DForm->clippingCtrl()->updateClipping(true);

        ui->obj3dViewer->fitDisplay(voxel_2d_scalar);
        /*
        Node* voxel_2d_scalar =
            (Node*)ui->tableWidget_result3d_op2_list->item(row, 1)->data(Qt::UserRole).toULongLong();
        viewOnPlane(voxel_2d_scalar);
        */
    });

    connect(ui->tableWidget_result3d_op2_list->horizontalHeader(), &QHeaderView::sectionClicked, this,
            [this](int) { ui->tableWidget_result3d_op2_list->setSortingEnabled(true); });

    ui->tableWidget_result3d_op2_list->setAcceptDrops(true);
    ui->tableWidget_result3d_op2_list->setDropCallback([this](const QStringList& paths) {
        QStringList file_paths;
        for (const QString& filePath : paths) {
            QFileInfo info(filePath);

            if (info.isDir()) {
                QDirIterator it(filePath, QDir::Files, QDirIterator::NoIteratorFlags);
                while (it.hasNext()) {
                    QString subFilePath = it.next();

                    QString ext = QFileInfo(subFilePath).suffix().toLower();
                    if (ext == "op2") {
                        file_paths << subFilePath;
                    }
                }
            }
            else {
                QString ext = QFileInfo(filePath).suffix().toLower();
                if (ext == "op2") {
                    file_paths << filePath;
                }
            }
        }

        m_3DForm->allLoad(file_paths, !m_3DForm->settingGuiCtrl()->isDragDropOp2Open());
    });
}

class SelectAllLineEdit : public QLineEdit {
public:
    using QLineEdit::QLineEdit;    // 親コンストラクタも継承
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        QLineEdit::mouseDoubleClickEvent(event);    // 親側も呼ぶ
        selectAll();
    }
};

void ResultCtrl::add2dResultList(Node* voxel_2d_scalar, bool d_section, int row)
{
    auto table_widget = ui->tableWidget_result3d_op2_list;

    int column = 0;

    /// View
    auto checkBox_text = new QTableWidgetItem();    /// ソート用
    checkBox_text->setText("");
    table_widget->setItem(row, column, checkBox_text);
    QWidget*     checkBoxWidget = new QWidget();
    QCheckBox*   checkBox       = new QCheckBox(checkBoxWidget);
    QHBoxLayout* layout         = new QHBoxLayout(checkBoxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    table_widget->setCellWidget(row, column++, checkBoxWidget);
    checkBox->setChecked(true);
    connect(checkBox, &QCheckBox::toggled, this, [this, voxel_2d_scalar, checkBox_text](bool checked) {
        if (checked) {
            voxel_2d_scalar->show();
            /*
            if (m_last_view_on_plane_path.empty()) {
                m_3DForm->clippingCtrl()->updateClipping(true);
            }
            else {
                auto plane = voxel_2d_scalar->matrix().toPlane();
                plane      = Planef(-plane.normal(), plane.origin());    /// 断面側が逆なので合わせる
                if (plane != m_last_view_on_plane) {
                    m_3DForm->clippingCtrl()->reset(true);
                    m_last_view_on_plane_path.clear();
                    m_last_view_on_plane.set(0, 0, 0, 0);
                }
            }
            */
            m_3DForm->clippingCtrl()->updateClipping(true);
            ui->obj3dViewer->update();

            ui->tableWidget_result3d_op2_list->setSortingEnabled(false);
            checkBox_text->setText("");
        }
        else {
            voxel_2d_scalar->hide();
            ui->obj3dViewer->update();

            ui->tableWidget_result3d_op2_list->setSortingEnabled(false);
            checkBox_text->setText(" ");
        }
    });

    /// Name
    QTableWidgetItem* noItem = new QTableWidgetItem(QString::fromStdWString(voxel_2d_scalar->name()));
    noItem->setFlags(noItem->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, noItem);
    noItem->setData(Qt::UserRole, reinterpret_cast<qulonglong>(voxel_2d_scalar));
    noItem->setToolTip(QString::fromStdWString(voxel_2d_scalar->name()));

    /// 面直
    QPushButton* along_plane = new QPushButton();
    along_plane->setText("exec");
    along_plane->setStyleSheet("QPushButton { padding: 0px; margin: 0px; }");
    table_widget->setCellWidget(row, column++, along_plane);
    connect(along_plane, &QPushButton::clicked, this,
            [this, voxel_2d_scalar]() { viewOnPlane(voxel_2d_scalar, true); });

    /// 面直 - 3D
    QPushButton* along_plane_3d = new QPushButton();
    along_plane_3d->setText("exec");
    along_plane_3d->setStyleSheet("QPushButton { padding: 0px; margin: 0px; }");
    table_widget->setCellWidget(row, column++, along_plane_3d);
    connect(along_plane_3d, &QPushButton::clicked, this,
            [this, voxel_2d_scalar]() { viewOnPlane(voxel_2d_scalar, false); });

    /// SetMinMax
    QPushButton* set_minmax = new QPushButton();
    set_minmax->setText("set");
    set_minmax->setStyleSheet("QPushButton { padding: 0px; margin: 0px; }");
    table_widget->setCellWidget(row, column++, set_minmax);
    connect(set_minmax, &QPushButton::clicked, this, [this, voxel_2d_scalar]() { setMinMax(voxel_2d_scalar); });

    float min_value           = 0.0;
    float max_value           = 0.0;
    auto  voxel_2d_scalar_obj = voxel_2d_scalar->object<VoxelScalar>();
    if (voxel_2d_scalar_obj) {
        min_value = voxel_2d_scalar_obj->minValue();
        max_value = voxel_2d_scalar_obj->maxValue();
    }

    /// Min
    noItem = new QTableWidgetItem(QString::number(min_value, 'E', 5));
    noItem->setFlags(noItem->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, noItem);

    /// Max
    noItem = new QTableWidgetItem(QString::number(max_value, 'E', 5));
    noItem->setFlags(noItem->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, noItem);
    /*
    // Min
    QLineEdit* minEdit = new SelectAllLineEdit(QString::number(min_value, 'E', 5));
    minEdit->setReadOnly(true);    // 編集不可
    minEdit->setFrame(false);      // 枠なしでテーブルになじませる（好みで）
    table_widget->setCellWidget(row, column++, minEdit);

    // Max
    QLineEdit* maxEdit = new SelectAllLineEdit(QString::number(max_value, 'E', 5));
    maxEdit->setReadOnly(true);    // 編集不可
    maxEdit->setFrame(false);      // 枠なし
    table_widget->setCellWidget(row, column++, maxEdit);
    */

    /// Info
    QPushButton* info = new QPushButton();
    info->setText("info");
    info->setStyleSheet("QPushButton { padding: 0px; margin: 0px; }");
    table_widget->setCellWidget(row, column++, info);
    connect(info, &QPushButton::clicked, this, [this, voxel_2d_scalar]() { op2Detail(voxel_2d_scalar); });

    /// "Array X"
    if (d_section) {
        CustomSpin* cell_x = new CustomSpin();
        cell_x->setRange(1, 999);
        cell_x->setValue(ui->spinBox_result3d_array_x->value());
        cell_x->setContextMenuPolicy(Qt::NoContextMenu);
        auto text_item = new QTableWidgetItem();
        text_item->setText(QString::number(cell_x->value()).rightJustified(5, '0'));
        table_widget->setItem(row, column, text_item);
        table_widget->setCellWidget(row, column++, cell_x);
        connect(cell_x, &QSpinBox::valueChanged, this, [this, voxel_2d_scalar, cell_x, text_item](int value) {
            ui->tableWidget_result3d_op2_list->setSortingEnabled(
                false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
            text_item->setText(QString::number(cell_x->value()).rightJustified(5, '0'));
            applyOp2ArraySetting(voxel_2d_scalar, -1, true);
            // text_item->setText(QString::number(value));/// 勝手にソートされるのでやめる
        });
    }
    else {
        auto text_item = new QTableWidgetItem();
        text_item->setText("");    /// Dummyで、上のケース（SpinBoxあり）とはソートできるようにするため
        table_widget->setItem(row, column, text_item);
        column++;
    }

    /// "Array Y"
    if (d_section) {
        CustomSpin* cell_y = new CustomSpin();
        cell_y->setRange(1, 999);
        cell_y->setValue(ui->spinBox_result3d_array_y->value());
        cell_y->setContextMenuPolicy(Qt::NoContextMenu);
        auto text_item = new QTableWidgetItem();
        text_item->setText(QString::number(cell_y->value()).rightJustified(5, '0'));
        table_widget->setItem(row, column, text_item);
        table_widget->setCellWidget(row, column++, cell_y);
        connect(cell_y, &QSpinBox::valueChanged, this, [this, voxel_2d_scalar, cell_y, text_item](int value) {
            ui->tableWidget_result3d_op2_list->setSortingEnabled(
                false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
            text_item->setText(QString::number(cell_y->value()).rightJustified(5, '0'));
            applyOp2ArraySetting(voxel_2d_scalar, -1, true);
        });
    }
    else {
        auto text_item = new QTableWidgetItem();
        text_item->setText("");    /// Dummyで、上のケース（SpinBoxあり）とはソートできるようにするため
        table_widget->setItem(row, column, text_item);
        column++;
    }

    /// "Vox X"
    if (d_section) {
        CustomDoubleSpin* vox_x = new CustomDoubleSpin();
        vox_x->setRange(0, 99999999);
        vox_x->setValue(ui->doubleSpinBox_result3d_voxel_size_x->value());
        vox_x->setContextMenuPolicy(Qt::NoContextMenu);
        auto text_item = new QTableWidgetItem();
        text_item->setText(QString::number(vox_x->value()).rightJustified(10, '0'));
        table_widget->setItem(row, column, text_item);
        table_widget->setCellWidget(row, column++, vox_x);
        connect(vox_x, &QDoubleSpinBox::valueChanged, this, [this, voxel_2d_scalar, vox_x, text_item](double value) {
            ui->tableWidget_result3d_op2_list->setSortingEnabled(
                false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
            text_item->setText(QString::number(vox_x->value()).rightJustified(10, '0'));
            applyOp2ArraySetting(voxel_2d_scalar, -1, true);
        });
    }
    else {
        auto text_item = new QTableWidgetItem();
        text_item->setText("");    /// Dummyで、上のケース（SpinBoxあり）とはソートできるようにするため
        table_widget->setItem(row, column, text_item);
        column++;
    }

    /// "Vox Y"
    if (d_section) {
        CustomDoubleSpin* vox_y = new CustomDoubleSpin();
        vox_y->setRange(0, 99999999);
        vox_y->setValue(ui->doubleSpinBox_result3d_voxel_size_y->value());
        vox_y->setContextMenuPolicy(Qt::NoContextMenu);
        auto text_item = new QTableWidgetItem();
        text_item->setText(QString::number(vox_y->value()).rightJustified(10, '0'));
        table_widget->setItem(row, column, text_item);
        table_widget->setCellWidget(row, column++, vox_y);
        connect(vox_y, &QDoubleSpinBox::valueChanged, this, [this, voxel_2d_scalar, vox_y, text_item](double value) {
            ui->tableWidget_result3d_op2_list->setSortingEnabled(
                false);    /// 勝手に順番変わるので無効にする。ヘッダ触ったら有効にする処理入れる
            text_item->setText(QString::number(vox_y->value()).rightJustified(10, '0'));
            applyOp2ArraySetting(voxel_2d_scalar, -1, true);
        });
    }
    else {
        auto text_item = new QTableWidgetItem();
        text_item->setText("");    /// Dummyで、上のケース（SpinBoxあり）とはソートできるようにするため
        table_widget->setItem(row, column, text_item);
        column++;
    }

    ///  "Folder"
    const std::wstring& file_path = voxel_2d_scalar->userAttributeString(L"File Path");

    QTableWidgetItem* folderItem =
        new QTableWidgetItem(QFileInfo(QString::fromStdWString(file_path)).absoluteDir().absolutePath());
    folderItem->setFlags(noItem->flags() & ~Qt::ItemIsEditable);
    table_widget->setItem(row, column++, folderItem);
    folderItem->setToolTip(folderItem->text());
}

void ResultCtrl::select2dVoxel(Node* voxel_node)
{
    if (m_voxel_3d == voxel_node) {
        return;
    }

    for (int row = 0; row < ui->tableWidget_result3d_op2_list->rowCount(); row++) {
        Node* node = (Node*)ui->tableWidget_result3d_op2_list->item(row, 1)->data(Qt::UserRole).toULongLong();
        if (node == voxel_node) {
            auto save_selection_mode = ui->tableWidget_result3d_op2_list->selectionMode();
            ui->tableWidget_result3d_op2_list->setSelectionMode(QAbstractItemView::SingleSelection);
            ui->tableWidget_result3d_op2_list->selectRow(row);
            ui->tableWidget_result3d_op2_list->setSelectionMode(save_selection_mode);
            return;
        }
    }
}

QString ResultCtrl::colormapMinLabel() const
{
    return ui->LE_result3d_min_value->text();
}

QString ResultCtrl::colormapMaxLabel() const
{
    return ui->LE_result3d_max_value->text();
}

void ResultCtrl::result2dListContextMenu(const QPoint& mouse_pos)
{
    /// 現在の選択を保持
    QItemSelectionModel* selection_model = ui->tableWidget_result3d_op2_list->selectionModel();
    QModelIndexList      selectedIndexes = selection_model->selectedRows();
    QList<int>           rows;
    for (const QModelIndex& index : selectedIndexes) {
        rows << index.row();
    }
    std::sort(rows.begin(), rows.end());

    /// メニュー作成
    QMenu    contextMenu;
    QAction* action_show = contextMenu.addAction("On");
    QAction* action_hide = contextMenu.addAction("Off");
    contextMenu.addSeparator();
    QAction* action_array = contextMenu.addAction("Array Setting");
    contextMenu.addSeparator();
    QAction* action_del = contextMenu.addAction("Delete");
    contextMenu.addSeparator();
    QAction* action_info = contextMenu.addAction("Information");

    /// 実行
    QAction* selectedAction = contextMenu.exec(mouse_pos);

    /// 選択を再現
    for (auto& row : rows) {
        QModelIndex index = ui->tableWidget_result3d_op2_list->model()->index(row, 0);
        selection_model->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    if (selectedAction == action_show) {
        result2dListShow(true, false);
    }
    else if (selectedAction == action_hide) {
        result2dListShow(false, false);
    }
    else if (selectedAction == action_array) {
        applyOp2Array(false, true);
    }
    else if (selectedAction == action_del) {
        deleteResult2dList(false);
    }
    else if (selectedAction == action_info) {
        op2Detail(nullptr);
    }
}

void ResultCtrl::result2dListShow(bool show, bool all, Node* except_node, bool update_view)
{
    std::vector<QCheckBox*> buttons;
    std::vector<QCheckBox*> buttons_except;

    std::vector<QTableWidgetItem*> buttons_item;
    std::vector<QTableWidgetItem*> buttons_except_item;

    // bool clear_clipping = show && !m_last_view_on_plane_path.empty();
    // bool same_plane     = true;

    if (all) {
        // same_plane = false;
        for (int row = 0; row < ui->tableWidget_result3d_op2_list->rowCount(); ++row) {
            Node* node = (Node*)ui->tableWidget_result3d_op2_list->item(row, 1)->data(Qt::UserRole).toULongLong();
            if (except_node == node) {
                node->setShowHierarchy(!show);
                buttons_except.emplace_back(
                    (QCheckBox*)ui->tableWidget_result3d_op2_list->cellWidget(row, 0)->findChild<QCheckBox*>());
                buttons_except_item.emplace_back(ui->tableWidget_result3d_op2_list->item(row, 0));
                continue;
            }
            node->setShowHierarchy(show);
            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_result3d_op2_list->cellWidget(row, 0)->findChild<QCheckBox*>());
            buttons_item.emplace_back(ui->tableWidget_result3d_op2_list->item(row, 0));
        }
    }
    else {
        QItemSelectionModel* selection_model = ui->tableWidget_result3d_op2_list->selectionModel();
        QModelIndexList      selectedIndexes = selection_model->selectedRows();
        QList<int>           rows;
        for (const QModelIndex& index : selectedIndexes) {
            rows << index.row();
        }
        std::sort(rows.begin(), rows.end());
        for (int row : rows) {
            Node* node = (Node*)ui->tableWidget_result3d_op2_list->item(row, 1)->data(Qt::UserRole).toULongLong();
            if (except_node == node) {
                node->setShowHierarchy(!show);
                buttons_except.emplace_back(
                    (QCheckBox*)ui->tableWidget_result3d_op2_list->cellWidget(row, 0)->findChild<QCheckBox*>());
                buttons_except_item.emplace_back(ui->tableWidget_result3d_op2_list->item(row, 0));
                continue;
            }
            /*
            if (clear_clipping && same_plane) {
                auto plane = node->matrix().toPlane();
                plane      = Planef(-plane.normal(), plane.origin());    /// 断面側が逆なので合わせる
                if (plane != m_last_view_on_plane) {
                    same_plane = false;
                }
            }*/
            node->setShowHierarchy(show);
            buttons.emplace_back(
                (QCheckBox*)ui->tableWidget_result3d_op2_list->cellWidget(row, 0)->findChild<QCheckBox*>());
            buttons_item.emplace_back(ui->tableWidget_result3d_op2_list->item(row, 0));
        }
    }

    ui->tableWidget_result3d_op2_list->setSortingEnabled(false);

    for (int ic = 0; ic < buttons.size(); ++ic) {
        auto& button      = buttons[ic];
        auto& button_item = buttons_item[ic];
        button->blockSignals(true);
        button->setChecked(show);
        button_item->setText(show ? "" : " ");
        button->blockSignals(false);
    }

    for (int ic = 0; ic < buttons_except.size(); ++ic) {
        auto& button      = buttons_except[ic];
        auto& button_item = buttons_except_item[ic];

        button->blockSignals(true);
        button->setChecked(!show);
        button_item->setText(!show ? "" : " ");
        button->blockSignals(false);
    }

    /*
    if (clear_clipping && !same_plane) {
        m_3DForm->clippingCtrl()->reset(true);
        m_last_view_on_plane_path.clear();
        m_last_view_on_plane.set(0, 0, 0, 0);
    }*/

    if (update_view) {
        m_3DForm->clippingCtrl()->updateClipping(true);
        ui->obj3dViewer->update();
    }
}

void ResultCtrl::deleteResult2dList(bool all)
{
    std::set<Node*> targets;

    if (all) {
        ui->tableWidget_result3d_op2_list->setRowCount(0);

        for (auto voxel_scalar : m_voxel_2d_list) {
            m_voxel_range_voxels.erase(voxel_scalar);
            targets.insert(voxel_scalar.ptr());
        }
        m_path_to_2dvoxel.clear();
        m_voxel_2d_list.clear();
    }
    else {
        QItemSelectionModel* selection_model = ui->tableWidget_result3d_op2_list->selectionModel();
        QModelIndexList      selectedIndexes = selection_model->selectedRows();
        QList<int>           rows;
        for (const QModelIndex& index : selectedIndexes) {
            Node* node =
                (Node*)ui->tableWidget_result3d_op2_list->item(index.row(), 1)->data(Qt::UserRole).toULongLong();
            targets.insert(node);

            rows << index.row();
        }
        std::vector<WeakRefPtr<Node>>       new_voxel_2d_list;
        std::map<QString, WeakRefPtr<Node>> new_path_to_2d;
        for (auto voxel : m_voxel_2d_list) {
            if (!targets.count(voxel.ptr())) {
                new_voxel_2d_list.emplace_back(voxel);
            }
            else {
                m_voxel_range_voxels.erase(voxel);
            }
        }
        for (auto& [path, node] : m_path_to_2dvoxel) {
            if (!targets.count(node.ptr())) {
                new_path_to_2d[path] = node;
            }
        }
        m_path_to_2dvoxel = new_path_to_2d;
        m_voxel_2d_list   = new_voxel_2d_list;

        std::sort(rows.begin(), rows.end(), std::greater<int>());
        for (int row : rows) {
            ui->tableWidget_result3d_op2_list->removeRow(row);
        }
    }

    if (targets.empty()) {
        return;
    }

    auto root_node = ui->obj3dViewer->sceneView()->sceneGraph()->rootNode();
    root_node->removeChild(targets);

    /// クリッピング領域変わるので必要
    m_3DForm->clippingCtrl()->reset(false);
    m_3DForm->clippingCtrl()->reset(true);
    ui->obj3dViewer->update();
}

void ResultCtrl::applyOp2Array(bool all, bool show_setting_dlg)
{
    /// Target
    std::set<int> targets;

    if (all) {
        for (int ic = 0; ic < ui->tableWidget_result3d_op2_list->rowCount(); ++ic) {
            QWidget* widget = ui->tableWidget_result3d_op2_list->cellWidget(ic, RESULT2DLIST_ARRY_X);
            if (widget) {
                targets.insert(ic);
            }
        }
    }
    else {
        QItemSelectionModel* selection_model = ui->tableWidget_result3d_op2_list->selectionModel();
        QModelIndexList      selectedIndexes = selection_model->selectedRows();
        for (const QModelIndex& index : selectedIndexes) {
            QWidget* widget = ui->tableWidget_result3d_op2_list->cellWidget(index.row(), RESULT2DLIST_ARRY_X);
            if (widget) {
                targets.insert(index.row());
            }
        }
    }

    if (targets.size() == 0) {
        return;
    }

    int    arry_x = ui->spinBox_result3d_array_x->value();
    int    arry_y = ui->spinBox_result3d_array_y->value();
    double cell_x = ui->doubleSpinBox_result3d_voxel_size_x->value();
    double cell_y = ui->doubleSpinBox_result3d_voxel_size_y->value();

    if (show_setting_dlg) {
        Op2ArraySettingDlg dlg(m_3DForm);

        if (targets.size() == 1) {
            int index = *targets.begin();
            arry_x    = ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(index, RESULT2DLIST_ARRY_X))->value();
            arry_y    = ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(index, RESULT2DLIST_ARRY_Y))->value();
            cell_x =
                ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(index, RESULT2DLIST_VOX_X))->value();
            cell_y =
                ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(index, RESULT2DLIST_VOX_Y))->value();
        }

        dlg.setArrayX(arry_x);
        dlg.setArrayY(arry_y);
        dlg.setVoxX(cell_x);
        dlg.setVoxY(cell_y);

        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        arry_x = dlg.arrayX();
        arry_y = dlg.arrayY();
        cell_x = dlg.voxX();
        cell_y = dlg.voxY();
    }

    ui->tableWidget_result3d_op2_list->setSortingEnabled(false);

    /// Setting
    for (auto target : targets) {
        ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_ARRY_X))->blockSignals(true);
        ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_ARRY_Y))->blockSignals(true);
        ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_VOX_X))
            ->blockSignals(true);
        ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_VOX_Y))
            ->blockSignals(true);

        ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_ARRY_X))->setValue(arry_x);
        ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_ARRY_Y))->setValue(arry_y);
        ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_VOX_X))->setValue(cell_x);
        ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_VOX_Y))->setValue(cell_y);
        ui->tableWidget_result3d_op2_list->item(target, RESULT2DLIST_ARRY_X)
            ->setText(QString::number(arry_x).rightJustified(5, '0'));
        ui->tableWidget_result3d_op2_list->item(target, RESULT2DLIST_ARRY_Y)
            ->setText(QString::number(arry_y).rightJustified(5, '0'));
        ui->tableWidget_result3d_op2_list->item(target, RESULT2DLIST_VOX_X)
            ->setText(QString::number(cell_x).rightJustified(10, '0'));
        ui->tableWidget_result3d_op2_list->item(target, RESULT2DLIST_VOX_Y)
            ->setText(QString::number(cell_y).rightJustified(10, '0'));

        Node* node = (Node*)ui->tableWidget_result3d_op2_list->item(target, 1)->data(Qt::UserRole).toULongLong();

        applyOp2ArraySetting(node, target, false);

        ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_ARRY_X))->blockSignals(false);
        ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_ARRY_Y))->blockSignals(false);
        ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_VOX_X))
            ->blockSignals(false);
        ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(target, RESULT2DLIST_VOX_Y))
            ->blockSignals(false);
    }

    if (ui->groupBox_result3d_specified_range->isChecked()
        || ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
        createVoxelRangeShape();
    }

    /// クリッピング領域変わるので必要
    m_3DForm->clippingCtrl()->reset(false);
    m_3DForm->clippingCtrl()->reset(true);

    ui->obj3dViewer->update();
}

void ResultCtrl::applyOp2ArraySetting(Node* node, int index, bool update_view)
{
    if (index < 0
        || (node != (Node*)ui->tableWidget_result3d_op2_list->item(index, 1)->data(Qt::UserRole).toULongLong())) {
        index = -1;
        for (int ic = 0; ic < ui->tableWidget_result3d_op2_list->rowCount(); ++ic) {
            if (node == (Node*)ui->tableWidget_result3d_op2_list->item(ic, 1)->data(Qt::UserRole).toULongLong()) {
                index = ic;
                break;
            }
        }
        if (index < 0) {
            return;
        }
    }

    if (setDSectionMatrix(node, index)) {
        auto itr_find = m_voxel_range_voxels.find(node);
        if (itr_find != m_voxel_range_voxels.end()) {
            m_voxel_range_voxels.erase(itr_find);

            if (update_view) {
                if (ui->groupBox_result3d_specified_range->isChecked()
                    || ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
                    createVoxelRangeShape();
                }
            }
        }
    }

    if (update_view) {
        ui->obj3dViewer->update();
    }
}

bool ResultCtrl::setDSectionMatrix(Node* voxel_2d_scalar, int list_index, int array_x, int array_y, double vox_x,
                                   double vox_y)
{
    /// ファイル名から座標を決める
    const QString& voxel_name = QString::fromStdWString(voxel_2d_scalar->name());

    QString symbol, symbol2, number, number2;
    if (!sectionName(voxel_name, symbol, number, symbol2, number2)) {
        return false;
    }
    if (symbol.compare("D", Qt::CaseInsensitive) != 0) {
        return false;
    }

    /// 移動量
    if (array_x < 0) {
        if (list_index >= 0) {
            array_x =
                ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(list_index, RESULT2DLIST_ARRY_X))->value();
            array_y =
                ((QSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(list_index, RESULT2DLIST_ARRY_Y))->value();
            vox_x = ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(list_index, RESULT2DLIST_VOX_X))
                        ->value()
                  / 1.0e3;
            vox_y = ((QDoubleSpinBox*)ui->tableWidget_result3d_op2_list->cellWidget(list_index, RESULT2DLIST_VOX_Y))
                        ->value()
                  / 1.0e3;
        }
        else {
            array_x = ui->spinBox_result3d_array_x->value();
            array_y = ui->spinBox_result3d_array_y->value();
            vox_x   = ui->doubleSpinBox_result3d_voxel_size_x->value() / 1.0e3;
            vox_y   = ui->doubleSpinBox_result3d_voxel_size_y->value() / 1.0e3;
        }
    }

    float x_move_val = array_x ? (float)(vox_x / (double)array_x) : 0;
    float y_move_val = array_y ? (float)(vox_y / (double)array_y) : 0;

    /// 移動
    int x_move = number2.toInt();
    if (x_move > 0) {
        x_move -= 1;
    }

    int y_move = 0;
    if (symbol2.size() == 1) {
        char c = symbol2[0].toLatin1();
        if (c >= 'A' && c <= 'Z') {
            y_move = c - 'A';    // 'A'～'Z' → 0～25
        }
        else if (c >= 'a' && c <= 'z') {
            y_move = c - 'a';    // 'a'～'z' → 0～25
        }
    }
    float eps = ui->obj3dViewer->sceneView()->sceneGraph()->lengthUnit().epsilonValidLength();

    Matrix4x4f matrix;

    if (number.toInt() == 45) {
        /// 移動
        Matrix4x4f trans;
        trans.translate((float)x_move * x_move_val, (float)y_move * y_move_val, 0);

        /// 回転
        Matrix4x4f rotate;
        rotate.rotateDegree(90.0f, Point3f(1, 0, 0));
        rotate.rotateDegree(45.0f, Point3f(0, 1, 0));

        matrix = trans * rotate;

        auto voxel_2d_scalar_obj = voxel_2d_scalar->object<VoxelScalar>();
        voxel_2d_scalar_obj->setBoundaryDeltaX(voxel_2d_scalar_obj->delta().x() * sqrt(2.0));

        /// 暫定 - セルがずれる場合とりあえず値を設定しておく
        float x_trans   = (float)x_move * x_move_val;
        int   m         = int(x_trans / voxel_2d_scalar_obj->delta().x());
        float nearest_x = m * voxel_2d_scalar_obj->delta().x();
        if (fabs(x_trans - nearest_x) > eps) {
            voxel_2d_scalar_obj->setBoundaryOrgX(-fabs(x_trans - nearest_x) * sqrt(2.0));
        }
        else {
            float y_trans   = (float)y_move * y_move_val;
            int   m         = int(y_trans / voxel_2d_scalar_obj->delta().y());
            float nearest_y = m * voxel_2d_scalar_obj->delta().y();
            if (fabs(y_trans - nearest_y) > eps) {
                voxel_2d_scalar_obj->setBoundaryOrgX(-fabs(y_trans - nearest_y) * sqrt(2.0));
            }
            else {
                voxel_2d_scalar_obj->setBoundaryOrgX(0);
            }
        }
    }
    else if (number.toInt() == 135) {
        /// 移動
        Matrix4x4f trans;
        trans.translate((float)(x_move + 1) * x_move_val, (float)y_move * y_move_val, 0);

        /// 回転
        Matrix4x4f rotate;
        rotate.rotateDegree(90.0f, Point3f(1, 0, 0));
        rotate.rotateDegree(135.0f, Point3f(0, 1, 0));

        matrix = trans * rotate;

        auto voxel_2d_scalar_obj = voxel_2d_scalar->object<VoxelScalar>();
        voxel_2d_scalar_obj->setBoundaryDeltaX(voxel_2d_scalar_obj->delta().x() * sqrt(2.0));

        /// 暫定 - セルがずれる場合とりあえず値を設定しておく
        float x_trans   = (float)x_move * x_move_val;
        int   m         = int(x_trans / voxel_2d_scalar_obj->delta().x());
        float nearest_x = m * voxel_2d_scalar_obj->delta().x();
        if (fabs(x_trans - nearest_x) > eps) {
            voxel_2d_scalar_obj->setBoundaryOrgX(-fabs(x_trans - nearest_x) * sqrt(2.0));
        }
        else {
            float y_trans   = (float)y_move * y_move_val;
            int   m         = int(y_trans / voxel_2d_scalar_obj->delta().y());
            float nearest_y = m * voxel_2d_scalar_obj->delta().y();
            if (fabs(y_trans - nearest_y) > eps) {
                voxel_2d_scalar_obj->setBoundaryOrgX(-fabs(y_trans - nearest_y) * sqrt(2.0));
            }
            else {
                voxel_2d_scalar_obj->setBoundaryOrgX(0);
            }
        }
    }

    if (voxel_2d_scalar->matrix() != matrix) {
        voxel_2d_scalar->setMatrix(matrix);
        voxel_2d_scalar->markBoxDirty();
        return true;
    }

    return false;
}

bool ResultCtrl::setShowOnlyVoxMaterialOnlyGUI(bool check)
{
    bool ret = ui->checkBox_result3d_show_only_in_voxel_material->isChecked();
    ui->checkBox_result3d_show_only_in_voxel_material->blockSignals(true);
    ui->checkBox_result3d_show_only_in_voxel_material->setChecked(check);
    ui->checkBox_result3d_show_only_in_voxel_material->blockSignals(false);
    return ret;
}

void ResultCtrl::opt3dDetail()
{
    if (!m_voxel_3d.isAlive()) {
        return;
    }

    // QString file_name = QString::fromStdWString(m_voxel_3d->userAttributeString(L"FileName"));

    VoxelScalar* voxel_scalar = (VoxelScalar*)m_voxel_3d->object();

    int   nx        = voxel_scalar->nX();
    int   ny        = voxel_scalar->nY();
    int   nz        = voxel_scalar->nZ();
    float min_value = voxel_scalar->minValue();
    float max_value = voxel_scalar->maxValue();

    float        mesh_size  = m_voxel_3d->userAttributeFloat(L"Mesh size");
    float        wavelength = m_voxel_3d->userAttributeFloat(L"Wavelength");
    float        theta      = m_voxel_3d->userAttributeFloat(L"Incident angle theta");
    float        phi        = m_voxel_3d->userAttributeFloat(L"Incident angle phi");
    float        psi        = m_voxel_3d->userAttributeFloat(L"Incident angle psi");
    int          excit      = m_voxel_3d->userAttributeInt(L"Excitation plane position (Z-axis coordinate)");
    std::wstring file_path  = m_voxel_3d->userAttributeString(L"File Path");

    QList<QPair<QString, QString>> texts;
    texts << qMakePair(QString("Nx"), QString::number(nx));
    texts << qMakePair(QString("Ny"), QString::number(ny));
    texts << qMakePair(QString("Nz"), QString::number(nz));
    texts << qMakePair(QString("E^2 Min"), QString::number(min_value, 'E', 5));
    texts << qMakePair(QString("E^2 Max"), QString::number(max_value, 'E', 5));
    texts << qMakePair(QString("Mesh size (um)"), QString::number(mesh_size * 1e9f));
    texts << qMakePair(QString("Wavelength (um)"), QString::number(wavelength * 1e9f));
    texts << qMakePair(QString("Incident angle theta (degree)"), QString::number(theta));
    texts << qMakePair(QString("Incident angle phi (degree)"), QString::number(phi));
    texts << qMakePair(QString("Incident angle psi (degree)"), QString::number(psi));
    texts << qMakePair(QString("Excitation plane position (Z-position)"), QString::number(excit));
    // texts << qMakePair(QString("File Path"), QString::fromStdWString(file_path));
    TextDialog dlg(texts, "opt detail", QString::fromStdWString(file_path), ui->resultTab);
    dlg.exec();
}

void ResultCtrl::op2Detail(Node* voxel_scalar)
{
    if (voxel_scalar == nullptr) {
        QItemSelectionModel* selection_model = ui->tableWidget_result3d_op2_list->selectionModel();
        QModelIndexList      selectedIndexes = selection_model->selectedRows();
        for (const QModelIndex& index : selectedIndexes) {
            Node* node =
                (Node*)ui->tableWidget_result3d_op2_list->item(index.row(), 1)->data(Qt::UserRole).toULongLong();
            if (node) {
                voxel_scalar = node;
                break;
            }
        }
        if (voxel_scalar == nullptr) {
            return;
        }
    }

    auto voxel_2d_scalar_obj = voxel_scalar->object<VoxelScalar>();
    if (!voxel_2d_scalar_obj) {
        return;
    }

    /// 1回だけなので
    /*
    QString file_name;
    for (int ic = 0; ic < ui->tableWidget_result3d_op2_list->rowCount(); ++ic) {
        if (voxel_scalar == (Node*)ui->tableWidget_result3d_op2_list->item(ic, 1)->data(Qt::UserRole).toULongLong()) {
            file_name = ui->tableWidget_result3d_op2_list->item(ic, 1)->text();
            break;
        }
    }*/

    int   nx        = voxel_2d_scalar_obj->nX();
    int   ny        = voxel_2d_scalar_obj->nY();
    float min_value = voxel_2d_scalar_obj->minValue();
    float max_value = voxel_2d_scalar_obj->maxValue();

    float        mesh_size  = voxel_scalar->userAttributeFloat(L"Mesh size");
    float        wavelength = voxel_scalar->userAttributeFloat(L"Wavelength");
    float        phi        = voxel_scalar->userAttributeFloat(L"Incident angle phi");
    int          ypos       = voxel_scalar->userAttributeInt(L"Excitation plane position (Y-axis coordinate)");
    std::wstring tmte       = voxel_scalar->userAttributeString(L"TMTE Flag");
    std::wstring file_path  = voxel_scalar->userAttributeString(L"File Path");

    QList<QPair<QString, QString>> texts;
    texts << qMakePair(QString("Nx"), QString::number(nx));
    texts << qMakePair(QString("Ny"), QString::number(ny));
    texts << qMakePair(QString("E^2 Min"), QString::number(min_value, 'E', 5));
    texts << qMakePair(QString("E^2 Max"), QString::number(max_value, 'E', 5));
    texts << qMakePair(QString("Mesh size (um)"), QString::number(mesh_size * 1e9f));
    texts << qMakePair(QString("Wavelength (um)"), QString::number(wavelength * 1e9f));
    texts << qMakePair(QString("Incident angle phi (degree)"), QString::number(phi));
    texts << qMakePair(QString("Excitation plane position (Y-position)"), QString::number(ypos));
    texts << qMakePair(QString("TM or TE"), QString::fromStdWString(tmte));
    // texts << qMakePair(QString("File Path"), QString::fromStdWString(file_path));
    TextDialog dlg(texts, "opt detail", QString::fromStdWString(file_path), ui->resultTab);
    dlg.exec();
}

void ResultCtrl::setMinMax(Node* node)
{
    auto voxel_2d_scalar_obj = node->object<VoxelScalar>();
    if (voxel_2d_scalar_obj) {
        float min_value = voxel_2d_scalar_obj->minValue();
        float max_value = voxel_2d_scalar_obj->maxValue();
        setColorBand(min_value, max_value);
    }
}

void ResultCtrl::viewOnPlane(Node* node, bool mode_2d)
{
    auto plane = node->matrix().toPlane();
    plane      = Planef(-plane.normal(), plane.origin());    /// 断面側が逆なので合わせる

    m_last_view_on_plane = plane;

    const std::wstring& file_path = node->userAttributeString(L"File Path");

    const auto& camera_dir = ui->obj3dViewer->sceneView()->cameraDir();

    bool any_min_priority = false;
    if (mode_2d) {
        if (m_last_view_on_plane_path == file_path) {
            /// ２回押したら反転させる
            if ((camera_dir - plane.normal()).length2() < 1.0e-12) {
                plane            = Planef(-plane.normal(), plane.origin());
                any_min_priority = true;
            }
        }
        else {
            /// 反転方向と一致ならそのまま反転維持
            if ((camera_dir + plane.normal()).length2() < 1.0e-12) {
                plane            = Planef(-plane.normal(), plane.origin());
                any_min_priority = true;
            }
        }
    }
    else {
        /// 今の方向に合わせる
        if (camera_dir * plane.normal() < 0) {
            plane            = Planef(-plane.normal(), plane.origin());
            any_min_priority = true;
        }
    }
    m_last_view_on_plane_path = file_path;

    result2dListShow(false, true, node, false);
    ui->checkBox_result3d_view->blockSignals(true);
    ui->checkBox_result3d_view->setChecked(false);
    ui->checkBox_result3d_view->blockSignals(false);
    ui->obj3dViewer->showOpt3D(false, false);

    if (mode_2d) {
        const auto& normal = plane.normal();
        if (normal == Point3f(-1, 0, 0)) {
            ui->obj3dViewer->sceneView()->rightView();
        }
        else if (normal == Point3f(1, 0, 0)) {
            ui->obj3dViewer->sceneView()->leftView();
        }
        else if (normal == Point3f(0, -1, 0)) {
            ui->obj3dViewer->sceneView()->frontView();
        }
        else if (normal == Point3f(0, 1, 0)) {
            ui->obj3dViewer->sceneView()->backView();
        }
        else if (normal == Point3f(0, 0, -1)) {
            ui->obj3dViewer->sceneView()->topView();
        }
        else if (normal == Point3f(0, 0, 1)) {
            ui->obj3dViewer->sceneView()->bottomView();
        }
        else {
            ui->obj3dViewer->sceneView()->dirView(plane.normal());
        }
    }
    this->m_3DForm->clippingCtrl()->applyPlanes({plane}, any_min_priority, mode_2d);
    ui->obj3dViewer->fitDisplay();
}

void ResultCtrl::viewOnPlaneNoChangeGUI(Node* node)
{
    auto plane = node->matrix().toPlane();
    plane      = Planef(-plane.normal(), plane.origin());    /// 断面側が逆なので合わせる

    viewOnPlaneNoChangeGUI(plane);
}

void ResultCtrl::viewOnPlaneNoChangeGUI(const Planef& plane)
{
    bool any_min_priority = false;

    const auto& normal = plane.normal();
    if (normal == Point3f(-1, 0, 0)) {
        ui->obj3dViewer->sceneView()->rightView();
    }
    else if (normal == Point3f(1, 0, 0)) {
        ui->obj3dViewer->sceneView()->leftView();
    }
    else if (normal == Point3f(0, -1, 0)) {
        ui->obj3dViewer->sceneView()->frontView();
    }
    else if (normal == Point3f(0, 1, 0)) {
        ui->obj3dViewer->sceneView()->backView();
    }
    else if (normal == Point3f(0, 0, -1)) {
        ui->obj3dViewer->sceneView()->topView();
    }
    else if (normal == Point3f(0, 0, 1)) {
        ui->obj3dViewer->sceneView()->bottomView();
    }
    else {
        ui->obj3dViewer->sceneView()->dirView(plane.normal());
    }
    this->m_3DForm->clippingCtrl()->applyClippingDirect({plane}, any_min_priority, true);
}

bool ResultCtrl::isShowInformationOnClick() const
{
    return ui->checkBox_result3d_show_information_on_click->isChecked();
}

int ResultCtrl::showInformationOnClickMsec() const
{
    return (int)(ui->doubleSpinBox_result3d_show_information_on_click->value() * 1000.0);
}

Point3f ResultCtrl::minDxDyDz()
{
    float dx = FLT_MAX;
    float dy = FLT_MAX;
    float dz = FLT_MAX;

    if (m_voxel_3d.isAlive()) {
        auto voxel_scalar = m_voxel_3d->object<VoxelScalar>();
        if (voxel_scalar) {
            dx = std::min(dx, (float)voxel_scalar->dX());
            dy = std::min(dy, (float)voxel_scalar->dY());
            dz = std::min(dz, (float)voxel_scalar->dZ());
        }
    }

    for (auto& op2 : m_voxel_2d_list) {
        if (op2.isAlive()) {
            auto voxel_scalar = op2->object<VoxelScalar>();
            if (voxel_scalar) {
                dx = std::min(dx, (float)voxel_scalar->dX());
                dy = std::min(dy, (float)voxel_scalar->dY());
                dz = std::min(dz, (float)voxel_scalar->dZ());
            }
        }
    }

    return Point3f(dx, dy, dz);
}

QString ResultCtrl::op2BaseName(const QString& op2_name, bool tm_te)
{
    /// IMX777_0.00_0.00_Y00700_530_TE_a0001.op2
    /// IMX777_0.00_0.00_Y00700_530_TE.op2
    /// IMX777_a0001_0.00_0.00_Y00700_530_TE ? (optはこうなっているので一応考慮）

    QStringList names = op2_name.split("_");
    if (names.isEmpty()) {
        return "";
    }

    /// 末尾拡張子除去
    auto& last_name = names[names.size() - 1];
    if (last_name.lastIndexOf(".op2", Qt::CaseInsensitive) == last_name.length() - 4) {
        last_name = last_name.mid(0, last_name.length() - 4);
    }

    /// TE or TM の後ろと前をつなぐ
    int tm_te_index = -1;
    for (int ic = names.size() - 1; ic >= 0; --ic) {
        if (names[ic] == "TE" || names[ic] == "TM") {
            tm_te_index = ic;
            break;
        }
    }
    /// フォーマット不正
    if (tm_te_index < 0) {
        return names[0];
    }

    QString ret_name;

    /// XXX_0.00_0.00_Y00700_530_TE
    for (int ic = 0; ic < tm_te_index - 4; ++ic) {
        ret_name += names[ic];
        ret_name += QString("_");
    }
    for (int ic = tm_te_index + (tm_te ? 0 : 1); ic < names.size(); ++ic) {
        ret_name += names[ic];
        ret_name += QString("_");
    }

    if (ret_name.size() == 0) {
        return names[0];
    }

    return ret_name.mid(0, ret_name.size() - 1);
}

QString ResultCtrl::optBaseName(const QString& opt_name, bool tm_te)
{
    /// IMX777_0.00_0.00_530_TE_a0001.op2 ?
    /// IMX777_0.00_0.00_530_TE.op2
    /// IMX777_a0001_0.00_0.00_530_TE
    ///
    QStringList names = opt_name.split("_");
    if (names.isEmpty()) {
        return "";
    }

    /// 末尾拡張子除去
    auto& last_name = names[names.size() - 1];
    if (last_name.lastIndexOf(".opt", Qt::CaseInsensitive) == last_name.length() - 4) {
        last_name = last_name.mid(0, last_name.length() - 4);
    }

    /// TE or TM の後ろと前をつなぐ
    int tm_te_index = -1;
    for (int ic = names.size() - 1; ic >= 0; --ic) {
        if (names[ic] == "TE" || names[ic] == "TM") {
            tm_te_index = ic;
            break;
        }
    }
    /// フォーマット不正
    if (tm_te_index < 0) {
        return names[0];
    }

    QString ret_name;

    /// XXX_0.00_0.00_530_TE
    for (int ic = 0; ic < tm_te_index - 3; ++ic) {
        ret_name += names[ic];
        ret_name += QString("_");
    }
    for (int ic = tm_te_index + (tm_te ? 0 : 1); ic < names.size(); ++ic) {
        ret_name += names[ic];
        ret_name += QString("_");
    }

    if (ret_name.size() == 0) {
        return names[0];
    }

    return ret_name.mid(0, ret_name.size() - 1);
}

bool ResultCtrl::isRelatedVoxOp(const QString& vox_name, const QString& op2opt_name)
{
    /// IMX777_a0001.vox
    /// IMX777.vox

    /// IMX777_0.00_0.00_XXX_530_TE_a0001.op2/opt
    /// IMX777_0.00_0.00_XXX_530_TE.op2/opt
    /// IMX777_a0001_0.00_0.00_XXX_530_TE.op2/opt

    QStringList vox_names = vox_name.split("_");

    QStringList op2opt_names = op2opt_name.split("_");

    if (vox_names.size() == 0 || op2opt_names.size() == 0) {
        return false;
    }

    /// 先頭
    if (vox_names[0].compare(op2opt_names[0], Qt::CaseInsensitive) != 0) {
        return false;
    }

    /// 残り(VoxNameがすべて含まれるか）
    int index = 1;
    for (int ic = 1; ic < vox_names.size(); ++ic) {
        bool check = false;
        for (index; index < op2opt_names.size(); ++index) {
            if (vox_names[ic].compare(op2opt_names[index], Qt::CaseInsensitive) == 0) {
                check = true;
                break;
            }
        }
        if (!check) {
            return false;
        }
    }

    return true;
}

QString ResultCtrl::relatedVoxOp(const QString&                                     op2opt_name,
                                 const std::map<QString, std::vector<VoxNameData>>& key_vox_name_data)
{
    /// IMX777_a0001.vox
    /// IMX777.vox

    /// IMX777_0.00_0.00_XXX_530_TE_a0001.op2/opt
    /// IMX777_0.00_0.00_XXX_530_TE.op2/opt
    /// IMX777_a0001_0.00_0.00_XXX_530_TE.op2/opt

    QStringList op2opt_names = op2opt_name.split("_");

    if (op2opt_names.size() == 0) {
        return "";
    }

    /// 先頭の一致（Keyは先頭名称）
    QString key      = op2opt_names[0];
    auto    itr_find = key_vox_name_data.find(key.toLower());
    if (itr_find == key_vox_name_data.end()) {
        return "";
    }

    std::vector<const VoxNameData*> targets;

    const std::vector<VoxNameData>& vox_name_datas = itr_find->second;
    for (const auto& vox_name_data : vox_name_datas) {
        const auto& vox_names = vox_name_data.m_vox_names;

        /// 残り(VoxNameがすべて含まれるか）
        bool related = true;
        int  index   = 1;
        for (int ic = 1; ic < vox_names.size(); ++ic) {
            bool check = false;
            for (index; index < op2opt_names.size(); ++index) {
                if (vox_names[ic].compare(op2opt_names[index], Qt::CaseInsensitive) == 0) {
                    check = true;
                    break;
                }
            }
            if (!check) {
                related = false;
                break;
            }
        }

        if (related) {
            targets.emplace_back(&vox_name_data);
        }
    }

    if (targets.size() > 1) {
        QFileInfo      file(op2opt_name);
        const QString& dir_path = file.absoluteDir().absolutePath();

        for (auto target : targets) {
            if (dir_path.compare(target->m_vox_folder, Qt::CaseInsensitive) == 0) {
                return target->m_vox_path;
            }
        }
    }

    if (targets.size() > 0) {
        return targets[0]->m_vox_path;
    }

    return "";
}

bool ResultCtrl::isRelatedVoxFdtd(const QString& vox_name, const QString& fdtd_name)
{
    /// IMX777_a0001.vox
    /// IMX777.vox
    ///
    /// IMX777.fdtd

    if (vox_name.startsWith(fdtd_name, Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}

QString ResultCtrl::relatedVoxFdtd(const QString& vox_name, const std::set<QString>& fdtd_names)
{
    if (fdtd_names.empty()) {
        return "";
    }
    QStringList vox_names = vox_name.split("_");

    if (vox_names.empty()) {
        return "";
    }

    QString check_name = vox_names[0];
    if (fdtd_names.count(check_name.toLower())) {
        return vox_names[0];
    }

    return "";
}

QString ResultCtrl::relatedVoxFdtd(const QString&                                      vox_name,
                                   const std::map<QString, std::vector<FdtdNameData>>& key_fdtd_name_data)
{
    QStringList vox_names = vox_name.split("_");

    if (vox_names.empty()) {
        return "";
    }

    QString check_name = vox_names[0];
    check_name         = check_name.toLower();

    auto itr_find = key_fdtd_name_data.find(check_name);
    if (itr_find == key_fdtd_name_data.end()) {
        return "";
    }

    const std::vector<FdtdNameData>& fdtd_name_datas = itr_find->second;
    if (fdtd_name_datas.size() > 1) {
        QFileInfo      file(vox_name);
        const QString& dir_path = file.absoluteDir().absolutePath();

        for (const auto& fdtd_name_data : fdtd_name_datas) {
            if (dir_path.compare(fdtd_name_data.m_folder, Qt::CaseInsensitive) == 0) {
                return fdtd_name_data.m_path;
            }
        }
    }

    if (fdtd_name_datas.size() > 0) {
        return fdtd_name_datas[0].m_path;
    }

    return "";
}

bool ResultCtrl::sectionName(const QString& file_name, QString& symbol, QString& number, QString& symbol2,
                             QString& number2)
{
    /// ファイル名から座標を決める
    QStringList voxel_names = file_name.split("_");
    if (voxel_names.size() < 3) {
        return false;
    }

    /// 後ろから 3 or 4 (先頭末尾に＿入る可能性ある？不明なので該当する場所とる)
    for (int ic = voxel_names.size() - 3; ic > 2; --ic) {
        const auto&               target = voxel_names.at(ic);
        static QRegularExpression re("([^0-9]+)([0-9]+)");
        QRegularExpressionMatch   match = re.match(target);
        if (match.hasMatch()) {
            symbol = match.captured(1);    /// 数字以外
            number = match.captured(2);    /// 数字部分

            if (symbol.compare("D", Qt::CaseInsensitive) == 0) {
                static QRegularExpression re2("([^0-9]+)([0-9]+)([^0-9]+)([0-9]+)");
                QRegularExpressionMatch   match2 = re2.match(target);
                if (match2.hasMatch()) {
                    symbol2 = match2.captured(3);    /// 数字以外
                    number2 = match2.captured(4);    /// 数字部分
                }
            }

            return true;
        }
    }

    return false;
}

void ResultCtrl::setResult3dView(bool show)
{
    ui->checkBox_result3d_view->setChecked(show);
}

bool ResultCtrl::isHideOnProjection() const
{
    return !ui->checkBox_result3d_hide_on_proj->isChecked();
}
