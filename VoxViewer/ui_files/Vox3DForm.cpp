#include "Vox3DForm.h"
#include "LightSettingDlg.h"
#include "ui_Vox3DForm.h"

#include <QColorDialog>
#include <QDockWidget>
#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>

#include "ClippingCtrl.h"
#include "DimensionCtrl.h"
#include "ResultCtrl.h"
#include "ResultStyleDefaultCombBox.h"
#include "SettingGuiCtrl.h"
#include "UtilityCtrl.h"

// このファイルの内容：　サンプルから流用　openGL描画のための基礎になるコード
// 分割 Vox3DFormSub01.cpp 　自分で今回付加したコード

#include <QDebug>

// void Vox3DForm::closeEvent(QCloseEvent *bar){ //×ボタンが押されても、画面を閉じないようにする
//     bar->ignore();
// }

class CustomDockWidget : public QDockWidget {
public:
    CustomDockWidget(const QString& title, QWidget* parent = nullptr) : QDockWidget(title, parent) {}

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override
    {
        if (isActiveWindow()) {
            if (minimumSize() != QSize(0, 0)) {
                setMinimumSize(QSize(0, 0));
            }
        }
        else {
            if (minimumSize() == QSize(0, 0)) {
                setMinimumSize(size());
            }
        }
        return QDockWidget::nativeEvent(eventType, message, result);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(249, 249, 249));    // 背景を白く描画
        QDockWidget::paintEvent(event);                     // 元の描画処理を呼び出す
    }
    void resizeEvent(QResizeEvent* event) override { QDockWidget::resizeEvent(event); }

    bool event(QEvent* event) override
    {
        if (isActiveWindow()) {
            if (minimumSize() != QSize(0, 0)) {
                setMinimumSize(QSize(0, 0));
            }
        }
        if (event->type() == QEvent::WindowActivate) {
            // qDebug() << "Widget became active.";
            setMinimumSize(QSize(0, 0));
            // アクティブになったときの処理
        }
        else if (event->type() == QEvent::Resize) {
            // qDebug() << "Widget became inactive.";
            if (isActiveWindow()) {
                setMinimumSize(QSize(0, 0));
            }
            // 非アクティブになったときの処理
        }
        if (event->type() == QEvent::ActivationChange) {
            if (isActiveWindow()) {
                // qDebug() << "Window became active.";
                //  ウィンドウがアクティブになったときの処理
                setMinimumSize(QSize(0, 0));
            }
            else {
                // qDebug() << "Window became inactive.";
                //  ウィンドウが非アクティブになったときの処理
            }
        }
        return QDockWidget::event(event);    // デフォルトの処理を呼び出す
    }
};

class CustomAcrollArea : public QScrollArea {
public:
    CustomAcrollArea(QWidget* parent = nullptr) : QScrollArea(parent) {}

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(249, 249, 249));    // 背景を白く描画
        QScrollArea::paintEvent(event);                     // 元の描画処理を呼び出す
    }

    void resizeEvent(QResizeEvent* event) override { QScrollArea::resizeEvent(event); }
};

class DockWidgetEventFilter : public QObject {
public:
    DockWidgetEventFilter(QDockWidget* dockWidget) : QObject(dockWidget), m_dockWidget(dockWidget) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (!m_dockWidget->isFloating()) {
            if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
                // サイズ変更や移動イベントをキャプチャして再描画を行う
                m_dockWidget->update();
                if (m_dockWidget->titleBarWidget()) {
                    m_dockWidget->titleBarWidget()->update();
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QDockWidget* m_dockWidget;
};

QScrollArea* createScrollableArea(QWidget* content, QWidget* parent)
{
    QScrollArea* scrollArea = new CustomAcrollArea(parent);
    scrollArea->setWidget(content);
    scrollArea->setWidgetResizable(true);    // 内容のサイズに応じてスクロールバーを表示

    return scrollArea;
}

Vox3DForm::Vox3DForm(QWidget* parent) : QWidget(parent), ui(new Ui::Vox3DForm)
{
    ui->setupUi(this);

    /// とりあえず非表示（機能OFF)
    // ui->pushButton_result3d_open_op2->setVisible(false);
    // ui->pushButton_open_op2->setVisible(false);

    ui->tabWidget_result->setCustomTabBar();
    ui->tabWidget_setting->setCustomTabBar();

    ui->lineEdit_voxpath->setDragDropFileExt({"vox"});
    ui->lineEdit_voxpath->setDropCallback([this](const QString& path) {
        if (QFileInfo(path).exists()) {
            voxpathLoad(path);
        }
    });
    ui->vox_fdtd_path_lineEdit->setDragDropFileExt({"fdtd"});
    ui->vox_fdtd_path_lineEdit->setDropCallback([this](const QString& path) {
        if (QFileInfo(path).exists()) {
            vox_fdtd_load(path);
        }
    });
    ui->lineEdit_optfile_path->setDragDropFileExt({"opt"});
    ui->lineEdit_optfile_path->setDropCallback([this](const QString& path) {
        if (QFileInfo(path).exists()) {
            optLoad(path);
        }
    });

    ui->comboBox_result3d_custom_setting_default->setVox3dForm(this);

    g_DEBUGMODE = 0;    // ユーザー配布用=0, 開発者デバッグ時だけ=1

    ui->mainTabs->setCurrentIndex(0);

    func_setGUIstartup();    // for-vox GUIデフォルト表示など
    // connect(ui->obj3dViewer, SIGNAL(func_signal_mousewheelZoom(float)), this, SLOT(func_slot_mousewheelZoom(float)));
    // connect(ui->obj3dViewer, SIGNAL(func_signal_mouseRotate(float, float)), this,
    //         SLOT(func_slot_mouseRotate(float, float)));
    // connect(ui->obj3dViewer, SIGNAL(func_signal_mouseDrag02(int, int)), this, SLOT(func_slot_mouseDrag02(int, int)));
    //  connect(ui->obj3dViewer, SIGNAL(func_signal_mousePress(float, float)), this, SLOT(func_slot_mousePress(float,
    //  float))); connect(ui->obj3dViewer, SIGNAL(func_signal_mouseRelease(float, float)), this,
    //  SLOT(func_slot_mouseRelease(float, float)));

    // QSplitter* splitter = new QSplitter(Qt::Vertical, ui->DimensionTab);
    // splitter->addWidget(ui->groupBox_create_dimension);
    // splitter->addWidget(ui->groupBox_create_auto_dimension);
    // ui->DimensionTab->layout()->addWidget(splitter);

    QSplitter* splitter = new QSplitter(Qt::Vertical, ui->AutoDimensionTab);
    splitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    splitter->addWidget(ui->tableWidget_create_auto_dimension);
    splitter->addWidget(ui->tableWidget_create_auto_dimension_text);
    ui->horizontalLayout_create_auto_dimension->addWidget(splitter);

    /// データ管理
    m_scene_graph = SceneGraph::createSceneGraph();

    /// 単位と許容値設定
    Core::LengthUnit unit;
    unit.set(Core::LengthUnit::Unit::Micrometer, 1.0e-6f, 1.0e-4f);
    m_scene_graph->setLengthUnit(unit);

    /// View初期化
    ui->obj3dViewer->initGLWidget(this);
    ui->obj3dViewer->setLightDir(LightSettingDlg::getLightDir(LightSettingDlg::RightUp), false);
    ui->obj3dViewer->setMouseZoomReverse(false);
    ui->checkBox_zoom_reverse->setChecked(ui->obj3dViewer->isMouseZoomReverse());

    connect(ui->pushButton_view_front, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->frontView(); });
    connect(ui->pushButton_view_back, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->backView(); });
    connect(ui->pushButton_view_left, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->leftView(); });
    connect(ui->pushButton_view_right, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->rightView(); });
    connect(ui->pushButton_view_top, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->topView(); });
    connect(ui->pushButton_view_bottom, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->bottomView(); });
    connect(ui->pushButton_view_axo, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->axoView(); });
    connect(ui->vox_comboBox_mouseAct, &QComboBox::currentTextChanged, this, [this](const QString& action) {
        if (action == m_mouse_rotate) {
            ui->obj3dViewer->setRotateMode();
        }
        else if (action == m_mouse_pan) {
            ui->obj3dViewer->setPanMode();
        }
        else {
            ui->obj3dViewer->setNoneMode();
        }
    });
    connect(ui->comboBox_projection, &QComboBox::currentTextChanged, this, [this](const QString& action) {
        if (action == m_projection_ortho) {
            ui->obj3dViewer->setProjectionOrtho();
        }
        else if (action == m_projection_perspective) {
            ui->obj3dViewer->setProjectionPerspective();
        }
        else {
            ui->obj3dViewer->setProjectionOrtho();
        }
    });
    connect(ui->checkBox_zoom_reverse, &QCheckBox::toggled, this,
            [this]() { ui->obj3dViewer->setMouseZoomReverse(ui->checkBox_zoom_reverse->isChecked()); });
    connect(ui->pushButton_rotateLeft, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->rotateLeft(); });
    connect(ui->pushButton_rotateRight, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->rotateRight(); });
    connect(ui->pushButton_rotateUp, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->rotateUp(); });
    connect(ui->pushButton_rotateDown, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->rotateDown(); });
    connect(ui->pushButton_axisLeft, &QPushButton::clicked, this, [this](bool /*checked*/) {
        QVector3D axis;
        if (ui->comboBox_axis->currentText() == "X") {
            if (ui->obj3dViewer->isZAxisRotateMode()) {
                return;
            }
            axis = QVector3D(1, 0, 0);
        }
        else if (ui->comboBox_axis->currentText() == "Y") {
            if (ui->obj3dViewer->isZAxisRotateMode()) {
                return;
            }
            axis = QVector3D(0, 1, 0);
        }
        else {
            axis = QVector3D(0, 0, 1);
        }
        ui->obj3dViewer->rotate(axis, 15.0);
    });
    connect(ui->pushButton_axisRight, &QPushButton::clicked, this, [this](bool /*checked*/) {
        QVector3D axis;
        if (ui->comboBox_axis->currentText() == "X") {
            if (ui->obj3dViewer->isZAxisRotateMode()) {
                return;
            }
            axis = QVector3D(1, 0, 0);
        }
        else if (ui->comboBox_axis->currentText() == "Y") {
            if (ui->obj3dViewer->isZAxisRotateMode()) {
                return;
            }
            axis = QVector3D(0, 1, 0);
        }
        else {
            axis = QVector3D(0, 0, 1);
        }
        ui->obj3dViewer->rotate(axis, -15.0);
    });

    connect(ui->pushButton_zoomIn, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->zoomIn(); });
    connect(ui->pushButton_zoomOut, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->zoomOut(); });
    connect(ui->pushButton_fit, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->fitDisplay(); });
    connect(ui->pushButton_init, &QPushButton::clicked, this, [this](bool /*checked*/) {
        if (QMessageBox::question(this, windowTitle(), "ファイルの読込はクリアせず、初期状態にします。")
            != QMessageBox::Yes) {
            return;
        }
        ui->obj3dViewer->suppressRender(true);
        m_dimension_ctrl->reset(true);
        m_dimension_ctrl->clearAutoDimensionMaterialNameChange();
        m_clipping_ctrl->reset(true);
        m_clipping_ctrl->reset(false);    /// クリップ位置戻すためfalseで呼ぶ
        m_result_ctrl->reset();

        if (ui->obj3dViewer->hasOptOp2()) {
            ui->checkBox_result3d_display_in_view->setChecked(ui->checkBox_result3d_display_in_view_auto->isChecked());
        }
        else {
            ui->checkBox_result3d_display_in_view->setChecked(false);
        }

        ui->checkBox_project_opt->setChecked(false);
        on_checkBox_project_opt_stateChanged(0);
        func_tableMaterialDefaultSet_matnameToColorCombo();
        on_pushButton_viewAllOn_clicked();
        voxMaterialDrawMode(ui->comboBox_voxel_draw_mode->currentIndex(), true);
        ui->checkBox_acolor->setChecked(true);
        ui->checkBox_acolor->setChecked(false);
        ui->obj3dViewer->axoView();
        ui->obj3dViewer->fitDisplay();
        ui->obj3dViewer->suppressRender(false);
    });

    connect(ui->pushButton_reset, &QPushButton::clicked, this, [this](bool /*checked*/) {
        if (QMessageBox::question(this, windowTitle(), "ファイルの読込をすべてクリアして、初期状態にします。")
            != QMessageBox::Yes) {
            return;
        }

        ui->obj3dViewer->suppressRender(true);

        on_pushButton_optClear_clicked();
        m_result_ctrl->deleteResult2dList(true);
        on_pushButton_voxpathClear_clicked();
        on_vox_fdtd_clear_pushButton_clicked();

        ui->checkBox_result3d_display_in_view->setChecked(false);

        m_dimension_ctrl->reset(true);
        m_dimension_ctrl->clearAutoDimensionMaterialNameChange();
        m_clipping_ctrl->reset(true);
        m_clipping_ctrl->reset(false);    /// クリップ位置戻すためfalseで呼ぶ
        m_result_ctrl->reset();
        ui->checkBox_project_opt->setChecked(false);
        on_checkBox_project_opt_stateChanged(0);
        func_tableMaterialDefaultSet_matnameToColorCombo();
        on_pushButton_viewAllOn_clicked();
        voxMaterialDrawMode(ui->comboBox_voxel_draw_mode->currentIndex(), true);
        ui->checkBox_acolor->setChecked(true);
        ui->checkBox_acolor->setChecked(false);
        ui->obj3dViewer->axoView();
        ui->obj3dViewer->fitDisplay();
        ui->obj3dViewer->suppressRender(false);
    });

    ui->checkBox_zaxis_rotate_mode->setChecked(false);
    ui->obj3dViewer->setZAxisRotateMode(ui->checkBox_zaxis_rotate_mode->isChecked());
    connect(ui->checkBox_zaxis_rotate_mode, &QCheckBox::toggled, this, [this](bool checked) {
        ui->obj3dViewer->setZAxisRotateMode(checked);
        if (checked) {
            ui->comboBox_axis->clear();
            ui->comboBox_axis->addItems({"Z"});
        }
        else {
            QString text = ui->comboBox_axis->currentText();
            ui->comboBox_axis->clear();
            ui->comboBox_axis->addItems({"X", "Y", "Z"});
            ui->comboBox_axis->setCurrentText(text);
        }
    });
    connect(ui->pushButton_LightSetting, &QPushButton::clicked, this, [this](bool /*checked*/) {
        LightSettingDlg dlg(this, ui->obj3dViewer);
        dlg.exec();
    });

    connect(ui->tableWidget_material, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        int matnum = ui->tableWidget_material->item(row, 1)->text().toInt();

        switch (ui->comboBox_doubleClick->currentIndex()) {
            case 0:    /// 対象のみ表示
                m_result_ctrl->suppressUpdateResult(true);
                on_pushButton_viewAllOff_clicked();
                for (int ic = 0; ic < ui->tableWidget_material->rowCount(); ++ic) {
                    QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 0);
                    QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
                    if (matnum == ui->tableWidget_material->item(row, 1)->text().toInt()) {
                        checkbox->setChecked(true);
                        break;
                    }
                }
                m_result_ctrl->suppressUpdateResult(false);
                break;
            case 1:    /// FitDisplay
                ui->obj3dViewer->fitDisplay(matnum);
                break;
            case 2:    /// 対象のみ表示 + FitDisplay
                m_result_ctrl->suppressUpdateResult(true);
                on_pushButton_viewAllOff_clicked();
                for (int ic = 0; ic < ui->tableWidget_material->rowCount(); ++ic) {
                    QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 0);
                    QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
                    if (matnum == ui->tableWidget_material->item(row, 1)->text().toInt()) {
                        checkbox->setChecked(true);
                    }
                }
                m_result_ctrl->suppressUpdateResult(false);
                ui->obj3dViewer->fitDisplay(matnum);
                break;
        }
    });

    // カスタムコンテキストメニューを有効化
    ui->tableWidget_material->setContextMenuPolicy(Qt::CustomContextMenu);

    // シグナルとスロットを接続
    QObject::connect(ui->tableWidget_material, &QTableWidget::customContextMenuRequested,
                     [this](const QPoint& pos) { voxMaterialContextMenu(pos); });

    QColor color(Qt::white);
    ui->pushButton_background_color->setStyleSheet(Vox3DForm::colorStyle(color));
    connect(ui->pushButton_background_color, &QPushButton::clicked, this, [this]() {
        QColor color         = ui->obj3dViewer->backgroundColor();
        QColor selectedColor = QColorDialog::getColor(color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_background_color->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            ui->obj3dViewer->setBackgroundColor(selectedColor);
        }
    });
    ui->checkBox_background_grad->setChecked(ui->obj3dViewer->backGroundColorGradient());
    connect(ui->checkBox_background_grad, &QCheckBox::toggled, this,
            [this]() { ui->obj3dViewer->setBackGroundColorGradient(ui->checkBox_background_grad->isChecked()); });

    /// Voxelの描画モード
    ui->comboBox_voxel_draw_mode->clear();
    ui->comboBox_voxel_draw_mode->addItems({"Shading", "Wireframe", "Shading+Wireframe", "None"});
    ui->comboBox_voxel_draw_mode->setCurrentIndex(0);
    // ui->obj3dViewer->setVoxelDrawMode(true, false);
    connect(ui->comboBox_voxel_draw_mode, &QComboBox::activated, this,
            [this](int index) { voxMaterialDrawMode(index, true); });
    QColor color_wire(Qt::black);
    ui->obj3dViewer->setVoxelWireframeColor(color_wire);
    ui->pushButton_voxel_wire_color->setStyleSheet(Vox3DForm::colorStyle(color_wire));
    connect(ui->pushButton_voxel_wire_color, &QPushButton::clicked, this, [this]() {
        QColor color         = ui->obj3dViewer->voxelWireframeColor();
        QColor selectedColor = QColorDialog::getColor(color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_voxel_wire_color->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            ui->obj3dViewer->setVoxelWireframeColor(selectedColor);
            ui->radioButton_voxel_wire_color_specify->setChecked(true);
        }
    });
    ui->radioButton_voxel_wire_color_shape->setChecked(false);
    ui->radioButton_voxel_wire_color_specify->setChecked(true);
    ui->obj3dViewer->setVoxelWireframeColorShape(false);
    connect(ui->radioButton_voxel_wire_color_shape, &QRadioButton::toggled, this, [this]() {
        ui->obj3dViewer->setVoxelWireframeColorShape(ui->radioButton_voxel_wire_color_shape->isChecked());
    });

    ui->obj3dViewer->setVoxelWireframeWidth(1);
    connect(ui->spinBox_voxel_wire_width, &QSpinBox::valueChanged, this,
            [this]() { ui->obj3dViewer->setVoxelWireframeWidth((float)ui->spinBox_voxel_wire_width->value()); });

    connect(ui->pushButton_copy_image, &QPushButton::clicked, this,
            [this]() { ui->obj3dViewer->imageCopyToClipboard(); });
    connect(ui->pushButton_save_image, &QPushButton::clicked, this, [this]() { ui->obj3dViewer->imageCopyToFile(); });

    QColor color_opt(Qt::black);
    ui->obj3dViewer->setProjectOptBaseColor(color_opt);
    ui->pushButton_voxel_proj_result_color->setStyleSheet(Vox3DForm::colorStyle(color_opt));
    connect(ui->pushButton_voxel_proj_result_color, &QPushButton::clicked, this, [this]() {
        QColor color         = ui->obj3dViewer->projectOptBaseColor();
        QColor selectedColor = QColorDialog::getColor(color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_voxel_proj_result_color->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            ui->obj3dViewer->setProjectOptBaseColor(selectedColor);
            ui->radioButton_voxel_proj_result_specify_color->setChecked(true);
        }
    });
    ui->radioButton_voxel_proj_result_shape_color->setChecked(false);
    ui->radioButton_voxel_proj_result_specify_color->setChecked(true);
    ui->obj3dViewer->setProjectOptBaseColorShape(false);
    connect(ui->radioButton_voxel_proj_result_shape_color, &QRadioButton::toggled, this, [this]() {
        ui->obj3dViewer->setProjectOptBaseColorShape(ui->radioButton_voxel_proj_result_shape_color->isChecked());
    });

    /// Clipping GUI
    m_clipping_ctrl = new ClippingCtrl(this, ui);

    /// Dimension GUI
    m_dimension_ctrl = new DimensionCtrl(this, ui);

    /// Result GUI
    m_result_ctrl = new ResultCtrl(this, ui);

    /// Utility GUI
    m_utility_ctrl = new UtilityCtrl(this, ui);

    /// ビューのショートカットキーがフォーカスがないと使えないので、マウスが上にあれば使えるようにする
    /*    ui->mainTabs->installEventFilter(this);
        ui->VoxMaterialTab->installEventFilter(this);
        ui->tableWidget_material->installEventFilter(this);
        ui->ClippingTab->installEventFilter(this);
        ui->DimensionTab->installEventFilter(this);
        ui->tableWidget_create_dimension_list->installEventFilter(this);
        ui->textEdit_create_auto_dimension->installEventFilter(this);
        ui->horizontalSlider_ClipX->installEventFilter(this);
        ui->doubleSpinBox_ClipXMax->installEventFilter(this);
        ui->doubleSpinBox_ClipXMin->installEventFilter(this);
        ui->horizontalSlider_ClipY->installEventFilter(this);
        ui->doubleSpinBox_ClipYMax->installEventFilter(this);
        ui->doubleSpinBox_ClipYMin->installEventFilter(this);
        ui->horizontalSlider_ClipZ->installEventFilter(this);
        ui->doubleSpinBox_ClipZMax->installEventFilter(this);
        ui->doubleSpinBox_ClipZMin->installEventFilter(this);
        ui->horizontalSlider_ClipAny->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyMax->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyMin->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyDirX->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyDirY->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyDirZ->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyPosX->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyPosY->installEventFilter(this);
        ui->doubleSpinBox_ClipAnyPosZ->installEventFilter(this);
        ui->frame->installEventFilter(this);
        */
    installEventFiltersForAllUiMembers(this);

    /// 最後に設定（設定反映があるので最後）
    /// Setting Gui
    m_setting_gui_ctrl = new SettingGuiCtrl(this, ui);

    updateEverything();

    /// ドッキング可能にする（寸法と断面を同時につかいたいので）

    std::vector<QWidget*> tabs = {ui->VoxMaterialTab, ui->ClippingTab, ui->DimensionTab, ui->AutoDimensionTab,
                                  ui->resultTab,      ui->UtilityTab,  ui->SettingTab};

    for (int i = 0; i < tabs.size(); ++i) {
        QWidget* tabPage  = tabs[i];
        QString  tabTitle = ui->mainTabs->tabText(ui->mainTabs->indexOf(tabPage));

        CustomDockWidget* dockWidget = new CustomDockWidget(tabTitle, (QMainWindow*)parent);
        dockWidget->setObjectName("dockwidget_" + tabTitle);    /// 配置とか状態を保存するので必要
        dockWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        dockWidget->setWidget(tabPage);
        dockWidget->setMinimumSize(QSize(0, 0));

        QPalette palette = dockWidget->palette();
        palette.setColor(QPalette::Window, QColor(249, 249, 249));
        dockWidget->setPalette(palette);
        dockWidget->setBackgroundRole(QPalette::Window);

        DockWidgetEventFilter* filter = new DockWidgetEventFilter(dockWidget);
        dockWidget->installEventFilter(filter);

        ((QMainWindow*)parent)->addDockWidget(Qt::RightDockWidgetArea, dockWidget);
        m_dock_widgets << dockWidget;
    }

    for (int ic = 0; ic < m_dock_widgets.size() - 1; ++ic) {
        ((QMainWindow*)parent)->tabifyDockWidget(m_dock_widgets[ic], m_dock_widgets[ic + 1]);
    }
    if (m_dock_widgets.size() > 0) {
        m_dock_widgets[0]->raise();
    }

    ui->mainTabs->setParent(nullptr);
    ui->mainTabs->hide();
    ui->frame_2->hide();

    /// サイズ設定
    ((QMainWindow*)parent)->resize(1400, 800);

    /// スクロールエリアにする（最初にやるとサイズ調整がおかしいのでShowの後にする）
    QTimer::singleShot(0, this, [this]() {
        for (int ic = 0; ic < m_dock_widgets.size(); ++ic) {
            QScrollArea* scrollArea = new CustomAcrollArea(m_dock_widgets[ic]);
            scrollArea->setWidget(m_dock_widgets[ic]->widget());
            scrollArea->setWidgetResizable(true);
            m_dock_widgets[ic]->setWidget(scrollArea);
        }
    });

    QTimer::singleShot(0, this, [this]() {
        /// 初期状態を保持
        m_setting_gui_ctrl->saveMainWindowInitState();

        /// 保存した状態を読み込み
        m_setting_gui_ctrl->readMainWindowState();
    });
}

void Vox3DForm::installEventFiltersForAllUiMembers(QObject* parent)
{
    // 親オブジェクトに対してinstallEventFilterを適用
    if (QWidget* widget = qobject_cast<QWidget*>(parent)) {
        widget->installEventFilter(this);
    }

    // 子オブジェクトを再帰的に処理
    for (QObject* child : parent->children()) {
        installEventFiltersForAllUiMembers(child);
    }
}

bool Vox3DForm::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != ui->obj3dViewer) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            /// マウスカーソルがビュー内にあるかどうかを判定
            QPoint cursorPos = QCursor::pos();
            QPoint widgetPos = ui->obj3dViewer->mapFromGlobal(cursorPos);
            if (ui->obj3dViewer->rect().contains(widgetPos)) {
                // 特定のウィジェットでキーイベントを処理
                // QCoreApplication::sendEvent(ui->obj3dViewer, keyEvent);
                ui->obj3dViewer->keyPressEvent(keyEvent);
                return true;    // イベントを処理済みとしてマーク
            }
        }
    }
    // デフォルトのイベント処理を行う
    return QWidget::eventFilter(obj, event);
}

Vox3DForm::~Vox3DForm()
{
    delete m_clipping_ctrl;
    delete m_dimension_ctrl;
    delete m_setting_gui_ctrl;
    delete m_result_ctrl;
    delete m_utility_ctrl;
    delete ui;
}

QOpenGLWidget* Vox3DForm::openGLWidget()
{
    return ui->obj3dViewer;
}

void Vox3DForm::closeProcess()
{
    /// 終了直前の処理
    /// 　※先に特にOpenGL周りのデータを開放しておかないと終了に時間がかかるので必要
    ///
    ui->obj3dViewer->suppressRender(true);
    on_pushButton_optClear_clicked();
    m_result_ctrl->deleteResult2dList(true);
    on_pushButton_voxpathClear_clicked();
    // on_vox_fdtd_clear_pushButton_clicked();
}

void Vox3DForm::updateEverything()
{
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    ui->obj3dViewer->setPerspective(ui->persVerticalAngleSpin->value(), ui->persNearSpin->value(),
                                    ui->persFarSpin->value());

    ui->obj3dViewer->setLookAt(
        QVector3D(ui->lookEyeXSpin->value(), ui->lookEyeYSpin->value(), ui->lookEyeZSpin->value()),
        QVector3D(ui->lookCenterXSpin->value(), ui->lookCenterYSpin->value(), ui->lookCenterZSpin->value()),
        QVector3D(ui->lookUpXSpin->value(), ui->lookUpYSpin->value(), ui->lookUpZSpin->value()));

    ui->obj3dViewer->setTranslation(ui->translateXSpin->value(), ui->translateYSpin->value(),
                                    ui->translateZSpin->value());

    ui->obj3dViewer->setScale(ui->scaleSpin->value());

    ui->obj3dViewer->setRotation(ui->rotateAngleSpin->value(), ui->rotateXSpin->value(), ui->rotateYSpin->value(),
                                 ui->rotateZSpin->value());

    ui->obj3dViewer->setLighting(
        QVector3D(ui->lightPosXSpin->value(), ui->lightPosYSpin->value(), ui->lightPosZSpin->value()),
        QVector3D(ui->lightKd1Spin->value(), ui->lightKd2Spin->value(), ui->lightKd3Spin->value()),
        QVector3D(ui->lightLd1Spin->value(), ui->lightLd2Spin->value(), ui->lightLd3Spin->value()));

    ui->obj3dViewer->update();    // have to call this manually (for better performance)
#endif
}

void Vox3DForm::on_persVerticalAngleSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_persNearSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_persFarSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookEyeXSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookEyeYSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookEyeZSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookCenterXSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookCenterYSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookCenterZSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookUpXSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookUpYSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lookUpZSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_scaleSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_translateXSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_translateYSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_translateZSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_rotateAngleSpin_valueChanged(double arg1)
{
    ui->rotateAngleSlider->setValue(arg1 * 10);
    if (g_flag_notRedraw == 1) {
        return;    // 以下の処理しない場合
    }

    updateEverything();
}

void Vox3DForm::on_rotateXSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    if (g_flag_notRedraw == 1) {
        return;    // 以下の処理しない場合
    }

    updateEverything();
}

void Vox3DForm::on_rotateYSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    if (g_flag_notRedraw == 1) {
        return;    // 以下の処理しない場合
    }

    updateEverything();
}

void Vox3DForm::on_rotateZSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    if (g_flag_notRedraw == 1) {
        return;    // 以下の処理しない場合
    }

    updateEverything();
}

void Vox3DForm::on_lightPosXSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lightPosYSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lightPosZSpin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

// void Vox3DForm::on_actionAbout_Qt_triggered()
//{
//     qApp->aboutQt();
// }

//-start- org
// void Vox3DForm::on_action_Open_triggered()
//{
//-start- org
//    updateEverything();

//    QVector<QOpenGLTriangle3D> triangles;
//    QStringList comments;

//    QString file = QFileDialog::getOpenFileName(this,
//                                                "Open Object",
//                                                QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0),
//                                                "*.obj");

//    if(QFile(file).exists())
//    {
//        QObj3dReader().parseObjFile(file, comments, triangles);
//        ui->obj3dViewer->setTriangles(triangles);
//        qDebug() << "[DEBUG]Vox3DForm::on_action_Open_triggered"; //triangles" << triangles;
//    }
//-end- org

//    on_pushButton_voxpathLoad_clicked();    // for-vox Voxファイル読み込み　openGL描画する。
//}
//-end- org

// void Vox3DForm::on_actionExit_triggered()
//{
//     close();
// }

void Vox3DForm::on_persVerticalAngleSlider_valueChanged(int value)
{
    ui->persVerticalAngleSpin->setValue(double(value) / 100.0);
}

void Vox3DForm::on_persNearSlider_valueChanged(int value)
{
    ui->persNearSpin->setValue(double(value) / 100.0);
}

void Vox3DForm::on_persFarSlider_valueChanged(int value)
{
    ui->persFarSpin->setValue(double(value) / 100.0);
}

void Vox3DForm::on_lightKd1Spin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lightKd2Spin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lightKd3Spin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lightLd1Spin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lightLd2Spin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_lightLd3Spin_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateEverything();
}

void Vox3DForm::on_textureFileEdit_returnPressed()
{
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    ui->obj3dViewer->setTextureFile(ui->textureFileEdit->text());
    updateEverything();
#endif
}

void Vox3DForm::on_browseTextureBtn_pressed()
{
    QString file = QFileDialog::getOpenFileName(
        this, "Open Texture", QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0),
        "*.jpg *.png *.tif *.bmp");
    ui->textureFileEdit->setText(file);
    on_textureFileEdit_returnPressed();
}

void Vox3DForm::on_rotateAngleSlider_valueChanged(int value)
{
    ui->rotateAngleSpin->setValue(double(value) / 10.0);
}

QSet<QDockWidget*> Vox3DForm::tabActiveWidgets()
{
    QSet<QDockWidget*> current_docks;

    QMainWindow* main_window = (QMainWindow*)parentWidget();

    QMap<Qt::DockWidgetArea, QList<QDockWidget*>> areaDockMap;
    for (auto* dock : main_window->findChildren<QDockWidget*>()) {
        Qt::DockWidgetArea area = main_window->dockWidgetArea(dock);
        areaDockMap[area].append(dock);
    }

    /// TabBarのCurrent
    QSet<QString> current_tab_text;
    auto          tabBars = main_window->findChildren<QTabBar*>();
    for (auto* tabBar : tabBars) {
        /// タブバーのタブ数を確認
        int count         = tabBar->count();
        int current_index = tabBar->currentIndex();
        if (0 <= current_index && current_index < count) {
            const QString& tabText = tabBar->tabText(current_index);
            current_tab_text << tabText;
        }
    }

    /// 各DockWidgetAreaごとにアクティブなDockWidgetを探す
    for (auto area : areaDockMap.keys()) {
        const auto& docks = areaDockMap[area];
        for (auto dock : docks) {
            if (current_tab_text.contains(dock->windowTitle())) {
                current_docks << dock;
                // DEBUG() << dock->windowTitle();
                break;
            }
        }
    }

    return current_docks;
}

QDockWidget* Vox3DForm::clippingDockWidget()
{
    for (auto& dock_widget : m_dock_widgets) {
        if (dock_widget->widget() == ui->ClippingTab) {
            return dock_widget;
        }
    }
    return nullptr;
}

QDockWidget* Vox3DForm::dimensionDockWidget()
{
    for (auto& dock_widget : m_dock_widgets) {
        if (dock_widget->widget() == ui->DimensionTab) {
            return dock_widget;
        }
    }
    return nullptr;
}
