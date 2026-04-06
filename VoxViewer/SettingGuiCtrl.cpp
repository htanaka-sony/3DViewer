#include "SettingGuiCtrl.h"
#include "DimensionCtrl.h"
#include "ResultCtrl.h"

#include "LightSettingDlg.h"
#include "OptPerformanceSetting.h"
#include "Vox3DForm.h"
#include "ui_Vox3DForm.h"

#include <QColorDialog>
#include <QDockWidget>
#include <QMainWindow>

#ifdef _WIN32
    #include <Windows.h>
#endif

SettingGuiCtrl::SettingGuiCtrl(Vox3DForm* parent, Ui::Vox3DForm* ui_) : m_parent(parent), ui(ui_)
{
    m_init_geometry_setting.reset(new SettingsWrapper(SettingsWrapper::Mode::UseQMap));

    /// connect前
    resetDefault(true);
    resetDefaultOtherSettings(true);

    /// 設定値読込
    readMouseAct();
    readMouseZAxisFix();
    readMouseZoomReverse();
    readLightDir();
    readClipAnyPreset();
    readProjection();
    readBackGroundColor();
    readBackGroundColorGradient();

    readPickSnap();
    readPickSnapType();

    readDimensionCreateType();
    readDimensionColor();
    readDimensionSize();
    readDimensionLineWidth();
    readDimensionTextDrag();
    readDimensionDispUnit();
    readDimensionDispName();
    readDimensionEdgeType();

    readAutoDimensionCreateType();
    readAutoDimensionColor();
    readAutoDimensionSize();
    readAutoDimensionLineWidth();
    readAutoDimensionTextDrag();
    readAutoDimensionDispUnit();
    readAutoDimensionDispMat();
    readAutoDimensionDispValue();
    readAutoDimensionExtend();
    readAutoDimensionContinuous();
    readAutoDimensionAlongPlane();
    readAutoDimensionSection();
    readAutoDimensionTextAlign();
    readAutoDimensionDispName();
    readAutoDimensionEdgeType();

    readVoxelDrawMode();
    readVoxelWireColor();
    readVoxelWireColorShape();
    readVoxelWireWidth();
    readVoxelListDoubleClick();

    readVoxelOptBaseColor();
    readVoxelOptBaseColorShape();

    readResultColormapMode();
    readResultStyle();
    readResultAutoColorbar();
    readResultDisplayPriority();
    readResultApplyColorMinMax();
    readResultShowInformationOnClick();
    readResultShowInformationOnClickMSec();

    readCPUPriority();
    applyCPUPriority();

    readOpenAllOp2Open();
    readDragDropOp2Open();
    readDragDropFolderOpen();

    readOp2HideArraySetting();

    //// connect
    connect(ui->pushButton_apply_current_setting, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(m_parent, m_parent->windowTitle(),
                                  "起動時のデフォルト設定を、起動中のビューアの設定に反映します。")
            != QMessageBox::Yes) {
            return;
        }
        applyDefault();
    });
    connect(ui->pushButton_reset_default_setting, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(m_parent, m_parent->windowTitle(), "起動時のデフォルト設定を、初期設定に戻します。")
            != QMessageBox::Yes) {
            return;
        }
        resetDefault(false);
    });

    connect(ui->vox_comboBox_mouseAct_default, &QComboBox::currentIndexChanged, this, &SettingGuiCtrl::saveMouseAct);
    connect(ui->checkBox_zaxis_rotate_mode_default, &QCheckBox::toggled, this, &SettingGuiCtrl::saveMouseZAxisFix);
    connect(ui->checkBox_zoom_reverse_default, &QCheckBox::toggled, this, &SettingGuiCtrl::saveMouseZoomReverse);
    connect(ui->pushButton_LightSetting_default, &QPushButton::clicked, this, [this]() {
        LightSettingDlg dlg(m_parent, m_light_dir);
        dlg.exec();
        if (m_light_dir != dlg.getLightDir()) {
            m_light_dir = dlg.getLightDir();
            saveLightDir();
        }
    });
    connect(ui->comboBox_any_clip_preset_default, &QComboBox::currentIndexChanged, this,
            &SettingGuiCtrl::saveClipAnyPreset);

    connect(ui->comboBox_projection_default, &QComboBox::currentIndexChanged, this, &SettingGuiCtrl::saveProjection);

    ui->pushButton_background_color_default->setStyleSheet(Vox3DForm::colorStyle(m_backgound_color));
    connect(ui->pushButton_background_color_default, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_backgound_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_background_color_default->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            m_backgound_color = selectedColor;
            saveBackGroundColor();
        }
    });
    connect(ui->checkBox_background_grad_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveBackGroundColorGradient);

    connect(ui->checkBox_create_dimension_snap_default, &QCheckBox::toggled, this, &SettingGuiCtrl::savePickSnap);
    connect(ui->comboBox_ceate_dimension_snap_default, &QComboBox::currentIndexChanged, this,
            &SettingGuiCtrl::savePickSnapType);

    connect(ui->comboBox_create_dimension_type_default, &QComboBox::currentIndexChanged, this,
            &SettingGuiCtrl::saveDimensionCreateType);
    ui->pushButton_create_dimension_color_default->setStyleSheet(Vox3DForm::colorStyle(m_dimension_color));
    connect(ui->pushButton_create_dimension_color_default, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_dimension_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_create_dimension_color_default->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            m_dimension_color = selectedColor;
            saveDimensionColor();
        }
    });
    connect(ui->spinBox_create_dimension_size_default, &QSpinBox::valueChanged, this,
            &SettingGuiCtrl::saveDimensionSize);
    connect(ui->spinBox_create_dimension_linewidth_default, &QSpinBox::valueChanged, this,
            &SettingGuiCtrl::saveDimensionLineWidth);
    connect(ui->checkBox_create_dimension_text_drag_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveDimensionTextDrag);
    connect(ui->checkBox_create_dimension_unit_disp_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveDimensionDispUnit);
    connect(ui->comboBox_create_dimension_edge_type_default, &QComboBox::currentIndexChanged, this,
            &SettingGuiCtrl::saveDimensionEdgeType);
    connect(ui->checkBox_create_dimension_disp_name_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveDimensionDispName);

    connect(ui->comboBox_create_auto_dimension_type_default, &QComboBox::currentIndexChanged, this,
            &SettingGuiCtrl::saveAutoDimensionCreateType);
    ui->pushButton_create_auto_dimension_color_default->setStyleSheet(Vox3DForm::colorStyle(m_auto_dimension_color));
    connect(ui->pushButton_create_auto_dimension_color_default, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_auto_dimension_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_create_auto_dimension_color_default->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            m_auto_dimension_color = selectedColor;
            saveAutoDimensionColor();
        }
    });
    connect(ui->spinBox_create_auto_dimension_size_default, &QSpinBox::valueChanged, this,
            &SettingGuiCtrl::saveAutoDimensionSize);
    connect(ui->spinBox_create_auto_dimension_linewidth_default, &QSpinBox::valueChanged, this,
            &SettingGuiCtrl::saveAutoDimensionLineWidth);
    connect(ui->checkBox_create_auto_dimension_text_drag_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionTextDrag);
    connect(ui->checkBox_create_auto_dimension_unit_disp_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionDispUnit);
    connect(ui->checkBox_create_auto_dimension_disp_material_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionDispMat);
    connect(ui->checkBox_create_auto_dimension_disp_value_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionDispValue);
    connect(ui->checkBox_create_auto_dimension_extend_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionExtend);
    connect(ui->checkBox_create_auto_dimension_continue_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionContinuous);
    connect(ui->checkBox_create_auto_dimension_along_plane_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionAlongPlane);
    connect(ui->checkBox_create_auto_dimension_section_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionSection);
    connect(ui->comboBox_create_auto_dimension_text_align_default, &QComboBox::currentIndexChanged, this,
            &SettingGuiCtrl::saveAutoDimensionTextAlign);
    connect(ui->comboBox_create_auto_dimension_edge_type_default, &QComboBox::currentIndexChanged, this,
            &SettingGuiCtrl::saveAutoDimensionEdgeType);
    connect(ui->checkBox_create_auto_dimension_disp_name_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveAutoDimensionDispName);

    connect(ui->comboBox_voxel_draw_mode_default, &QComboBox::currentTextChanged, this,
            &SettingGuiCtrl::saveVoxelDrawMode);
    ui->pushButton_voxel_wire_color_default->setStyleSheet(Vox3DForm::colorStyle(m_voxel_wire_color));
    connect(ui->pushButton_voxel_wire_color_default, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_voxel_wire_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_voxel_wire_color_default->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            m_voxel_wire_color = selectedColor;
            saveVoxelWireColor();
        }
    });
    connect(ui->radioButton_voxel_wire_color_shape_default, &QRadioButton::toggled, this,
            &SettingGuiCtrl::saveVoxelWireColorShape);
    connect(ui->spinBox_voxel_wire_width_default, &QSpinBox::valueChanged, this, &SettingGuiCtrl::saveVoxelWireWidth);
    connect(ui->comboBox_doubleClick_default, &QComboBox::currentTextChanged, this,
            &SettingGuiCtrl::saveVoxelListDoubleClick);

    ui->pushButton_voxel_proj_result_color_default->setStyleSheet(Vox3DForm::colorStyle(m_voxel_opt_base_color));
    connect(ui->pushButton_voxel_proj_result_color_default, &QPushButton::clicked, this, [this]() {
        QColor selectedColor = QColorDialog::getColor(m_voxel_opt_base_color, (QWidget*)this->parent(), "色を選択");
        if (selectedColor.isValid()) {
            ui->pushButton_voxel_proj_result_color_default->setStyleSheet(Vox3DForm::colorStyle(selectedColor));
            m_voxel_opt_base_color = selectedColor;
            saveVoxelOptBaseColor();
        }
    });
    connect(ui->radioButton_voxel_proj_result_shape_color_default, &QRadioButton::toggled, this,
            &SettingGuiCtrl::saveVoxelOptBaseColorShape);

    connect(ui->rb_result3d_cmode_LOG_default, &QRadioButton::toggled, this, &SettingGuiCtrl::saveResultColormapMode);
    connect(ui->comboBox_result3d_custom_setting_default, &QComboBox::currentTextChanged, this,
            &SettingGuiCtrl::saveResultStyle);
    connect(ui->checkBox_result3d_display_in_view_auto_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveResultAutoColorbar);
    connect(ui->checkBox_result3d_display_priority_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveResultDisplayPriority);
    connect(ui->checkBox_result3d_apply_color_minmax_default, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveResultApplyColorMinMax);

    ui->doubleSpinBox_result3d_show_information_on_click_default->setEnabled(
        ui->checkBox_result3d_show_information_on_click_default->isChecked());
    ui->label_result3d_show_information_on_click_msec_default->setEnabled(
        ui->checkBox_result3d_show_information_on_click_default->isChecked());
    connect(ui->checkBox_result3d_show_information_on_click_default, &QCheckBox::toggled, this, [this]() {
        ui->doubleSpinBox_result3d_show_information_on_click_default->setEnabled(
            ui->checkBox_result3d_show_information_on_click_default->isChecked());
        ui->label_result3d_show_information_on_click_msec_default->setEnabled(
            ui->checkBox_result3d_show_information_on_click_default->isChecked());
        saveResultShowInformationOnClick();
    });
    connect(ui->doubleSpinBox_result3d_show_information_on_click_default, &QDoubleSpinBox::valueChanged, this,
            &SettingGuiCtrl::saveResultShowInformationOnClickMSec);

    /// パフォーマンス設定
    connect(ui->pushButton_result3d_opt_memory_setting, &QPushButton::clicked, this, [this]() {
        OptPerformanceSetting dlg(m_parent, this);
        dlg.exec();
    });

    /// Other
    connect(ui->checkBox_open_all_op2_open, &QCheckBox::toggled, this, &SettingGuiCtrl::saveOpenAllOp2Open);
    connect(ui->checkBox_drag_and_drop_op2_open, &QCheckBox::toggled, this, &SettingGuiCtrl::saveDragDropOp2Open);
    connect(ui->checkBox_drag_and_drop_folder_open, &QCheckBox::toggled, this, &SettingGuiCtrl::saveDragDropFolderOpen);
    connect(ui->pushButton_reset_others_default_setting, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(m_parent, m_parent->windowTitle(),
                                  "動作設定(パフォーマンス・その他)を、初期設定に戻します。")
            != QMessageBox::Yes) {
            return;
        }
        resetDefaultOtherSettings(false);
    });
    connect(ui->comboBox_cpu_priority, &QComboBox::currentTextChanged, this, [this]() {
        saveCPUPriority();
        applyCPUPriority();
    });
    connect(ui->checkBox_op2_load_dsection_dialog_hide, &QCheckBox::toggled, this,
            &SettingGuiCtrl::saveOp2HideArraySetting);

    /// デフォルトを反映
    applyDefault();
}

SettingGuiCtrl::~SettingGuiCtrl() {}

void SettingGuiCtrl::applyDefault()
{
    auto dimension_ctrl = m_parent->dimensionCtrl();

    m_parent->suppressUpdata();
    ui->obj3dViewer->suppressRender(true);

    /// 現在値を反映
    ui->vox_comboBox_mouseAct->setCurrentIndex(ui->vox_comboBox_mouseAct_default->currentIndex());
    ui->checkBox_zaxis_rotate_mode->setChecked(ui->checkBox_zaxis_rotate_mode_default->isChecked());
    ui->checkBox_zoom_reverse->setChecked(ui->checkBox_zoom_reverse_default->isChecked());
    ui->obj3dViewer->setLightDir(LightSettingDlg::getLightDir((LightSettingDlg::LightMode)m_light_dir), false);
    ui->comboBox_any_clip_preset->setCurrentIndex(ui->comboBox_any_clip_preset_default->currentIndex());
    ui->comboBox_any_clip_preset->activated(ui->comboBox_any_clip_preset_default->currentIndex());

    ui->comboBox_projection->setCurrentIndex(ui->comboBox_projection_default->currentIndex());
    ui->pushButton_background_color->setStyleSheet(Vox3DForm::colorStyle(m_backgound_color));
    ui->obj3dViewer->setBackgroundColor(m_backgound_color);
    ui->checkBox_background_grad->setChecked(ui->checkBox_background_grad_default->isChecked());

    ui->checkBox_create_dimension_snap->setChecked(ui->checkBox_create_dimension_snap_default->isChecked());
    ui->comboBox_ceate_dimension_snap->setCurrentIndex(ui->comboBox_ceate_dimension_snap_default->currentIndex());

    ui->comboBox_create_dimension_type->setCurrentIndex(ui->comboBox_create_dimension_type_default->currentIndex());
    ui->pushButton_create_dimension_color->setStyleSheet(Vox3DForm::colorStyle(m_dimension_color));
    dimension_ctrl->setDimensionColor(m_dimension_color);
    ui->spinBox_create_dimension_size->setValue(ui->spinBox_create_dimension_size_default->value());
    ui->spinBox_create_dimension_linewidth->setValue(ui->spinBox_create_dimension_linewidth_default->value());
    ui->checkBox_create_dimension_text_drag->setChecked(ui->checkBox_create_dimension_text_drag_default->isChecked());
    ui->checkBox_create_dimension_unit_disp->setChecked(ui->checkBox_create_dimension_unit_disp_default->isChecked());
    ui->comboBox_create_dimension_edge_type->setCurrentIndex(
        ui->comboBox_create_dimension_edge_type_default->currentIndex());
    ui->checkBox_create_dimension_disp_name->setChecked(ui->checkBox_create_dimension_disp_name_default->isChecked());

    ui->comboBox_create_auto_dimension_type->setCurrentIndex(
        ui->comboBox_create_auto_dimension_type_default->currentIndex());
    ui->pushButton_create_auto_dimension_color->setStyleSheet(Vox3DForm::colorStyle(m_auto_dimension_color));
    dimension_ctrl->setAutoDimensionColor(m_auto_dimension_color);
    ui->spinBox_create_auto_dimension_size->setValue(ui->spinBox_create_auto_dimension_size_default->value());
    ui->spinBox_create_auto_dimension_linewidth->setValue(ui->spinBox_create_auto_dimension_linewidth_default->value());
    ui->checkBox_create_auto_dimension_text_drag->setChecked(
        ui->checkBox_create_auto_dimension_text_drag_default->isChecked());
    ui->checkBox_create_auto_dimension_unit_disp->setChecked(
        ui->checkBox_create_auto_dimension_unit_disp_default->isChecked());
    ui->checkBox_create_auto_dimension_disp_material->setChecked(
        ui->checkBox_create_auto_dimension_disp_material_default->isChecked());
    ui->checkBox_create_auto_dimension_disp_value->setChecked(
        ui->checkBox_create_auto_dimension_disp_value_default->isChecked());
    ui->checkBox_create_auto_dimension_extend->setChecked(
        ui->checkBox_create_auto_dimension_extend_default->isChecked());
    ui->checkBox_create_auto_dimension_continue->setChecked(
        ui->checkBox_create_auto_dimension_continue_default->isChecked());
    ui->checkBox_create_auto_dimension_along_plane->setChecked(
        ui->checkBox_create_auto_dimension_along_plane_default->isChecked());
    ui->checkBox_create_auto_dimension_section->setChecked(
        ui->checkBox_create_auto_dimension_section_default->isChecked());
    ui->comboBox_create_auto_dimension_text_align->setCurrentIndex(
        ui->comboBox_create_auto_dimension_text_align_default->currentIndex());
    ui->comboBox_create_auto_dimension_edge_type->setCurrentIndex(
        ui->comboBox_create_auto_dimension_edge_type_default->currentIndex());
    ui->checkBox_create_auto_dimension_disp_name->setChecked(
        ui->checkBox_create_auto_dimension_disp_name_default->isChecked());

    dimension_ctrl->createAutoDimensionGUIUpdate();

    ui->comboBox_voxel_draw_mode->setCurrentIndex(ui->comboBox_voxel_draw_mode_default->currentIndex());
    emit ui->comboBox_voxel_draw_mode->activated(ui->comboBox_voxel_draw_mode_default->currentIndex());
    ui->pushButton_voxel_wire_color->setStyleSheet(Vox3DForm::colorStyle(m_voxel_wire_color));
    ui->obj3dViewer->setVoxelWireframeColor(m_voxel_wire_color);
    ui->radioButton_voxel_wire_color_shape->setChecked(ui->radioButton_voxel_wire_color_shape_default->isChecked());
    ui->radioButton_voxel_wire_color_specify->setChecked(ui->radioButton_voxel_wire_color_specify_default->isChecked());
    ui->spinBox_voxel_wire_width->setValue(ui->spinBox_voxel_wire_width_default->value());
    ui->comboBox_doubleClick->setCurrentIndex(ui->comboBox_doubleClick_default->currentIndex());

    ui->pushButton_voxel_proj_result_color->setStyleSheet(Vox3DForm::colorStyle(m_voxel_opt_base_color));
    ui->obj3dViewer->setProjectOptBaseColor(m_voxel_opt_base_color);
    ui->radioButton_voxel_proj_result_shape_color->setChecked(
        ui->radioButton_voxel_proj_result_shape_color_default->isChecked());
    ui->radioButton_voxel_proj_result_specify_color->setChecked(
        ui->radioButton_voxel_proj_result_specify_color_default->isChecked());

    ui->rb_result3d_cmode_LOG->setChecked(ui->rb_result3d_cmode_LOG_default->isChecked());
    ui->rb_result3d_cmode_LINEAR->setChecked(ui->rb_result3d_cmode_LINEAR_default->isChecked());
    auto                style = ui->comboBox_result3d_custom_setting_default->currentText();
    QStandardItemModel* model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
    for (int ic = 0; ic < ui->comboBox_result3d_custom_setting->count(); ++ic) {
        auto color_item = model->item(ic);
        if (color_item && (color_item->flags() & Qt::NoItemFlags)) {
            continue;
        }

        auto text = ui->comboBox_result3d_custom_setting->itemText(ic);
        if (text.compare(style, Qt::CaseInsensitive) == 0) {
            ui->comboBox_result3d_custom_setting->setCurrentIndex(ic);
            break;
        }
    }
    ui->checkBox_result3d_display_in_view_auto->setChecked(
        ui->checkBox_result3d_display_in_view_auto_default->isChecked());
    ui->checkBox_result3d_display_priority->setChecked(ui->checkBox_result3d_display_priority_default->isChecked());
    ui->checkBox_result3d_apply_color_minmax->setChecked(ui->checkBox_result3d_apply_color_minmax_default->isChecked());
    ui->checkBox_result3d_show_information_on_click->setChecked(
        ui->checkBox_result3d_show_information_on_click_default->isChecked());
    ui->doubleSpinBox_result3d_show_information_on_click->setValue(
        ui->doubleSpinBox_result3d_show_information_on_click_default->value());

    m_parent->unsuppressUpdata();

    ui->obj3dViewer->suppressRender(false);
}

void SettingGuiCtrl::resetDefault(bool init)
{
    if (init) {
        /// Comboboxを合わせる
        ui->comboBox_voxel_draw_mode_default->clear();
        for (int ic = 0; ic < ui->comboBox_voxel_draw_mode->count(); ++ic) {
            ui->comboBox_voxel_draw_mode_default->addItem(ui->comboBox_voxel_draw_mode->itemText(ic));
        }
        ui->comboBox_doubleClick_default->clear();
        for (int ic = 0; ic < ui->comboBox_doubleClick->count(); ++ic) {
            ui->comboBox_doubleClick_default->addItem(ui->comboBox_doubleClick->itemText(ic));
        }
        ui->comboBox_projection_default->clear();
        for (int ic = 0; ic < ui->comboBox_projection->count(); ++ic) {
            ui->comboBox_projection_default->addItem(ui->comboBox_projection->itemText(ic));
        }
        ui->comboBox_create_dimension_edge_type_default->clear();
        for (int ic = 0; ic < ui->comboBox_create_dimension_edge_type->count(); ++ic) {
            ui->comboBox_create_dimension_edge_type_default->addItem(
                ui->comboBox_create_dimension_edge_type->itemText(ic));
        }
        ui->comboBox_create_auto_dimension_edge_type_default->clear();
        for (int ic = 0; ic < ui->comboBox_create_auto_dimension_edge_type->count(); ++ic) {
            ui->comboBox_create_auto_dimension_edge_type_default->addItem(
                ui->comboBox_create_auto_dimension_edge_type->itemText(ic));
        }

        /// 動的に変わるが一旦設定
        QStandardItemModel* custom_setting_default_model =
            new QStandardItemModel(ui->comboBox_result3d_custom_setting_default);
        ui->comboBox_result3d_custom_setting_default->setModel(custom_setting_default_model);

        QStandardItemModel* custom_setting_model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting->model();
        for (int ic = 0; ic < ui->comboBox_result3d_custom_setting->count(); ++ic) {
            QStandardItem* standardItem = new QStandardItem(ui->comboBox_result3d_custom_setting->itemText(ic));
            standardItem->setFlags(custom_setting_model->item(ic)->flags());
            custom_setting_default_model->appendRow(standardItem);
        }
    }

    /// デフォルトを設定
    ui->vox_comboBox_mouseAct_default->setCurrentIndex(0);
    ui->checkBox_zaxis_rotate_mode_default->setChecked(true);
    ui->checkBox_zoom_reverse_default->setChecked(false);
    ui->comboBox_any_clip_preset_default->setCurrentIndex(0);
    m_light_dir = (int)LightSettingDlg::RightUp;

    ui->comboBox_projection_default->setCurrentIndex(0);
    m_backgound_color.setRgb(180, 180, 180);
    ui->pushButton_background_color_default->setStyleSheet(Vox3DForm::colorStyle(m_backgound_color));
    ui->checkBox_background_grad_default->setChecked(false);

    ui->checkBox_create_dimension_snap_default->setChecked(true);
    ui->comboBox_ceate_dimension_snap_default->setCurrentIndex(2);

    ui->comboBox_create_dimension_type_default->setCurrentIndex(0);
    m_dimension_color = Qt::yellow;
    ui->pushButton_create_dimension_color_default->setStyleSheet(Vox3DForm::colorStyle(m_dimension_color));

    ui->spinBox_create_dimension_size_default->setValue(18);
    ui->spinBox_create_dimension_linewidth_default->setValue(2);
    ui->checkBox_create_dimension_text_drag_default->setChecked(true);
    ui->checkBox_create_dimension_unit_disp_default->setChecked(false);
    ui->comboBox_create_dimension_edge_type_default->setCurrentIndex(0);
    ui->checkBox_create_dimension_disp_name_default->setChecked(false);

    ui->comboBox_create_auto_dimension_type_default->setCurrentIndex(0);
    m_auto_dimension_color = Qt::yellow;
    ui->pushButton_create_auto_dimension_color_default->setStyleSheet(Vox3DForm::colorStyle(m_auto_dimension_color));

    ui->spinBox_create_auto_dimension_size_default->setValue(14);
    ui->spinBox_create_auto_dimension_linewidth_default->setValue(2);
    ui->checkBox_create_auto_dimension_text_drag_default->setChecked(false);
    ui->checkBox_create_auto_dimension_unit_disp_default->setChecked(false);
    ui->checkBox_create_auto_dimension_disp_material_default->setChecked(true);
    ui->checkBox_create_auto_dimension_disp_value_default->setChecked(true);
    ui->checkBox_create_auto_dimension_extend_default->setChecked(true);
    ui->checkBox_create_auto_dimension_continue_default->setChecked(false);
    ui->checkBox_create_auto_dimension_along_plane_default->setChecked(true);
    ui->checkBox_create_auto_dimension_section_default->setChecked(false);
    ui->comboBox_create_auto_dimension_text_align_default->setCurrentIndex(0);
    ui->comboBox_create_auto_dimension_edge_type_default->setCurrentIndex(0);
    ui->checkBox_create_auto_dimension_disp_name_default->setChecked(false);

    ui->comboBox_voxel_draw_mode_default->setCurrentIndex(2);
    m_voxel_wire_color = QColor(Qt::black);
    ui->pushButton_voxel_wire_color_default->setStyleSheet(Vox3DForm::colorStyle(m_voxel_wire_color));
    ui->radioButton_voxel_wire_color_shape_default->setChecked(false);
    ui->radioButton_voxel_wire_color_specify_default->setChecked(true);
    ui->spinBox_voxel_wire_width_default->setValue(1);
    ui->comboBox_doubleClick_default->setCurrentIndex(2);

    m_voxel_opt_base_color = QColor(Qt::black);
    ui->pushButton_voxel_proj_result_color_default->setStyleSheet(Vox3DForm::colorStyle(m_voxel_opt_base_color));
    ui->radioButton_voxel_proj_result_shape_color_default->setChecked(false);
    ui->radioButton_voxel_proj_result_specify_color_default->setChecked(true);

    ui->rb_result3d_cmode_LOG_default->setChecked(true);
    ui->rb_result3d_cmode_LINEAR_default->setChecked(false);
    ui->comboBox_result3d_custom_setting_default->setCurrentText(E2VIEWER_NAME);
    ui->checkBox_result3d_display_in_view_auto_default->setChecked(true);
    ui->checkBox_result3d_display_priority_default->setChecked(true);
    ui->doubleSpinBox_result3d_show_information_on_click->setValue(5.0);
    ui->checkBox_result3d_apply_color_minmax_default->setChecked(true);
    ui->checkBox_result3d_show_information_on_click_default->setChecked(true);

    /// 保存（connect済みなら上で保存される。上の保存で足りないもの）＆GUI更新
    if (!init) {
        saveLightDir();
        saveBackGroundColor();
        saveDimensionColor();
        saveAutoDimensionColor();
        saveVoxelWireColor();

        auto dimension_ctrl = m_parent->dimensionCtrl();
        dimension_ctrl->createAutoDimensionGUIUpdate();
    }
}

void SettingGuiCtrl::resetDefaultOtherSettings(bool init)
{
    if (init) {    /// 初回のみ
        ui->comboBox_cpu_priority->clear();
        // ui->comboBox_cpu_priority->addItem("Realtime"); /// 基本入れない（管理者権限起動なら可能だが）
        ui->comboBox_cpu_priority->addItem("High");
        ui->comboBox_cpu_priority->addItem("Above Normal");
        ui->comboBox_cpu_priority->addItem("Normal");
        ui->comboBox_cpu_priority->addItem("Below Normal");
        ui->comboBox_cpu_priority->addItem("Low");
    }
    else {    /// 別ダイアログ設定なのでinitで呼ばない。ボタン実行時にデフォルト戻すのみ
        if (readOptMemoryLimit() != optMemoryLimitDefault()) {
            saveOptMemoryLimit(optMemoryLimitDefault());
        }
        if (readOptMemorySize() != optMemorySizeDefault()) {
            saveOptMemorySize(optMemorySizeDefault());
        }
        if (readOptMemoryUnit() != optMemoryUnitDefault()) {
            saveOptMemoryUnit(optMemoryUnitDefault());
        }
        if (readOptMemoryMethod() != optMemoryMethodDefault()) {
            saveOptMemoryMethod(optMemoryMethodDefault());
        }
        if (readOptMemoryWarn() != optMemoryWarnDefault()) {
            saveOptMemoryWarn(optMemoryWarnDefault());
        }
    }

    ui->checkBox_open_all_op2_open->setChecked(true);
    ui->checkBox_drag_and_drop_op2_open->setChecked(false);
    ui->checkBox_drag_and_drop_folder_open->setChecked(true);

    ui->checkBox_op2_load_dsection_dialog_hide->setChecked(false);

    ui->comboBox_cpu_priority->setCurrentText("Normal");    /// Normal
}

void SettingGuiCtrl::readMouseAct()
{
    int mouse_act =
        m_setting_ctrl.getInt(SettingCtrl::CONFIG_MOUSE_ACT, ui->vox_comboBox_mouseAct_default->currentIndex());
    ui->vox_comboBox_mouseAct_default->setCurrentIndex(mouse_act);
}

void SettingGuiCtrl::saveMouseAct()
{
    int mouse_act = ui->vox_comboBox_mouseAct_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_MOUSE_ACT, mouse_act);
}

void SettingGuiCtrl::readMouseZAxisFix()
{
    int zaxis_fix = m_setting_ctrl.getInt(SettingCtrl::CONFIG_MOUSE_ZAXIS_FIX,
                                          ui->checkBox_zaxis_rotate_mode_default->isChecked() ? 1 : 0);
    ui->checkBox_zaxis_rotate_mode_default->setChecked(zaxis_fix != 0);
}

void SettingGuiCtrl::saveMouseZAxisFix()
{
    int zaxis_fix = ui->checkBox_zaxis_rotate_mode_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_MOUSE_ZAXIS_FIX, zaxis_fix);
}

void SettingGuiCtrl::readMouseZoomReverse()
{
    int zoom_reverse = m_setting_ctrl.getInt(SettingCtrl::CONFIG_MOUSE_ZOOM_REVERSE,
                                             ui->checkBox_zoom_reverse_default->isChecked() ? 1 : 0);
    ui->checkBox_zoom_reverse_default->setChecked(zoom_reverse != 0);
}

void SettingGuiCtrl::saveMouseZoomReverse()
{
    int zoom_reverse = ui->checkBox_zoom_reverse_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_MOUSE_ZOOM_REVERSE, zoom_reverse);
}

void SettingGuiCtrl::readLightDir()
{
    m_light_dir = m_setting_ctrl.getInt(SettingCtrl::CONFIG_LIGHT_DIR, m_light_dir);
    if (m_light_dir < 0 || LightSettingDlg::LightModeCount <= m_light_dir) {
        m_light_dir = LightSettingDlg::RightUp;
    }
}

void SettingGuiCtrl::saveLightDir()
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_LIGHT_DIR, m_light_dir);
}

void SettingGuiCtrl::readClipAnyPreset()
{
    int clip_preset =
        m_setting_ctrl.getInt(SettingCtrl::CONFIG_CLIP_PRESET, ui->comboBox_any_clip_preset_default->currentIndex());
    ui->comboBox_any_clip_preset_default->setCurrentIndex(clip_preset);
}

void SettingGuiCtrl::saveClipAnyPreset()
{
    int clip_preset = ui->comboBox_any_clip_preset_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_CLIP_PRESET, clip_preset);
}

void SettingGuiCtrl::readProjection()
{
    int projection =
        m_setting_ctrl.getInt(SettingCtrl::CONFIG_VIEW_PROJECTION, ui->comboBox_projection_default->currentIndex());
    ui->comboBox_projection_default->setCurrentIndex(projection);
}

void SettingGuiCtrl::saveProjection()
{
    int projection = ui->comboBox_projection_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VIEW_PROJECTION, projection);
}

void SettingGuiCtrl::readBackGroundColor()
{
    QColor default_color = m_backgound_color;
    default_color.name(QColor::HexArgb);
    QString hexargb =
        m_setting_ctrl.getString(SettingCtrl::CONFIG_VIEW_BACKGROUND_COLOR, default_color.name(QColor::HexArgb), true);
    m_backgound_color = QColor(hexargb);
}

void SettingGuiCtrl::saveBackGroundColor()
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_VIEW_BACKGROUND_COLOR, m_backgound_color.name(QColor::HexArgb));
}

void SettingGuiCtrl::readBackGroundColorGradient()
{
    int back_ground_color_grad = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VIEW_BACKGROUND_COLOR_GRAD,
                                                       ui->checkBox_background_grad_default->isChecked() ? 1 : 0);
    ui->checkBox_background_grad_default->setChecked(back_ground_color_grad != 0);
}

void SettingGuiCtrl::saveBackGroundColorGradient()
{
    int back_ground_color_grad = ui->checkBox_background_grad_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VIEW_BACKGROUND_COLOR_GRAD, back_ground_color_grad);
}

void SettingGuiCtrl::readPickSnap()
{
    int snap = m_setting_ctrl.getInt(SettingCtrl::CONFIG_PICK_SNAP,
                                     ui->checkBox_create_dimension_snap_default->isChecked() ? 1 : 0);
    ui->checkBox_create_dimension_snap_default->setChecked(snap != 0);
}

void SettingGuiCtrl::savePickSnap()
{
    int snap = ui->checkBox_create_dimension_snap_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_PICK_SNAP, snap);
}

void SettingGuiCtrl::readPickSnapType()
{
    int snap_type = m_setting_ctrl.getInt(SettingCtrl::CONFIG_PICK_SNAP_TYPE,
                                          ui->comboBox_ceate_dimension_snap_default->currentIndex());
    ui->comboBox_ceate_dimension_snap_default->setCurrentIndex(snap_type);
}

void SettingGuiCtrl::savePickSnapType()
{
    int snap_type = ui->comboBox_ceate_dimension_snap_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_PICK_SNAP_TYPE, snap_type);
}

void SettingGuiCtrl::readDimensionCreateType()
{
    int create_type = m_setting_ctrl.getInt(SettingCtrl::CONFIG_DIMENSION_CREATE_TYPE,
                                            ui->comboBox_create_dimension_type_default->currentIndex());
    ui->comboBox_create_dimension_type_default->setCurrentIndex(create_type);
}

void SettingGuiCtrl::saveDimensionCreateType()
{
    int create_type = ui->comboBox_create_dimension_type_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_DIMENSION_CREATE_TYPE, create_type);
}

void SettingGuiCtrl::readDimensionColor()
{
    QColor default_color = m_dimension_color;
    default_color.name(QColor::HexArgb);
    QString hexargb =
        m_setting_ctrl.getString(SettingCtrl::CONFIG_DIMENSION_COLOR, default_color.name(QColor::HexArgb), true);
    m_dimension_color = QColor(hexargb);
}

void SettingGuiCtrl::saveDimensionColor()
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_DIMENSION_COLOR, m_dimension_color.name(QColor::HexArgb));
}

void SettingGuiCtrl::readDimensionSize()
{
    int size =
        m_setting_ctrl.getInt(SettingCtrl::CONFIG_DIMENSION_SIZE, ui->spinBox_create_dimension_size_default->value());
    ui->spinBox_create_dimension_size_default->setValue(size);
}

void SettingGuiCtrl::saveDimensionSize()
{
    int size = ui->spinBox_create_dimension_size_default->value();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_DIMENSION_SIZE, size);
}

void SettingGuiCtrl::readDimensionLineWidth()
{
    int lw = m_setting_ctrl.getInt(SettingCtrl::CONFIG_DIMENSION_LINE_WIDTH,
                                   ui->spinBox_create_dimension_linewidth_default->value());
    ui->spinBox_create_dimension_linewidth_default->setValue(lw);
}

void SettingGuiCtrl::saveDimensionLineWidth()
{
    int lw = ui->spinBox_create_dimension_linewidth_default->value();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_DIMENSION_LINE_WIDTH, lw);
}

void SettingGuiCtrl::readDimensionEdgeType()
{
    int index = m_setting_ctrl.getInt(SettingCtrl::CONFIG_DIMENSION_EDGE_TYPE,
                                      ui->comboBox_create_dimension_edge_type_default->currentIndex());
    ui->comboBox_create_dimension_edge_type_default->setCurrentIndex(index);
}

void SettingGuiCtrl::saveDimensionEdgeType()
{
    int index = ui->comboBox_create_dimension_edge_type_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_DIMENSION_EDGE_TYPE, index);
}

void SettingGuiCtrl::readDimensionDispName()
{
    int disp = m_setting_ctrl.getInt(SettingCtrl::CONFIG_DIMENSION_DISP_NAME,
                                     ui->checkBox_create_dimension_disp_name_default->isChecked() ? 1 : 0);
    ui->checkBox_create_dimension_disp_name_default->setChecked(disp != 0);
}

void SettingGuiCtrl::saveDimensionDispName()
{
    int disp = ui->checkBox_create_dimension_disp_name_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_DIMENSION_DISP_NAME, disp);
}

void SettingGuiCtrl::readDimensionTextDrag()
{
    int text_drag = m_setting_ctrl.getInt(SettingCtrl::CONFIG_DIMENSION_TEXT_DRAG,
                                          ui->checkBox_create_dimension_text_drag_default->isChecked() ? 1 : 0);
    ui->checkBox_create_dimension_text_drag_default->setChecked(text_drag != 0);
}

void SettingGuiCtrl::saveDimensionTextDrag()
{
    int text_drag = ui->checkBox_create_dimension_text_drag_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_DIMENSION_TEXT_DRAG, text_drag);
}

void SettingGuiCtrl::readDimensionDispUnit()
{
    int text_drag = m_setting_ctrl.getInt(SettingCtrl::CONFIG_DIMENSION_DISP_UNIT,
                                          ui->checkBox_create_dimension_unit_disp_default->isChecked() ? 1 : 0);
    ui->checkBox_create_dimension_unit_disp_default->setChecked(text_drag != 0);
}

void SettingGuiCtrl::saveDimensionDispUnit()
{
    int text_drag = ui->checkBox_create_dimension_unit_disp_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_DIMENSION_DISP_UNIT, text_drag);
}

void SettingGuiCtrl::readAutoDimensionCreateType()
{
    int create_type = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_CREATE_TYPE,
                                            ui->comboBox_create_auto_dimension_type_default->currentIndex());
    ui->comboBox_create_auto_dimension_type_default->setCurrentIndex(create_type);
}

void SettingGuiCtrl::saveAutoDimensionCreateType()
{
    int create_type = ui->comboBox_create_auto_dimension_type_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_CREATE_TYPE, create_type);
}

void SettingGuiCtrl::readAutoDimensionColor()
{
    QColor default_color = m_auto_dimension_color;
    default_color.name(QColor::HexArgb);
    QString hexargb =
        m_setting_ctrl.getString(SettingCtrl::CONFIG_AUTO_DIMENSION_COLOR, default_color.name(QColor::HexArgb), true);
    m_auto_dimension_color = QColor(hexargb);
}

void SettingGuiCtrl::saveAutoDimensionColor()
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_AUTO_DIMENSION_COLOR, m_auto_dimension_color.name(QColor::HexArgb));
}

void SettingGuiCtrl::readAutoDimensionSize()
{
    int size = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_SIZE,
                                     ui->spinBox_create_auto_dimension_size_default->value());
    ui->spinBox_create_auto_dimension_size_default->setValue(size);
}

void SettingGuiCtrl::saveAutoDimensionSize()
{
    int size = ui->spinBox_create_auto_dimension_size_default->value();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_SIZE, size);
}

void SettingGuiCtrl::readAutoDimensionLineWidth()
{
    int lw = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_LINE_WIDTH,
                                   ui->spinBox_create_auto_dimension_linewidth_default->value());
    ui->spinBox_create_auto_dimension_linewidth_default->setValue(lw);
}

void SettingGuiCtrl::saveAutoDimensionLineWidth()
{
    int lw = ui->spinBox_create_auto_dimension_linewidth_default->value();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_LINE_WIDTH, lw);
}

void SettingGuiCtrl::readAutoDimensionEdgeType()
{
    int index = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_EDGE_TYPE,
                                      ui->comboBox_create_auto_dimension_edge_type_default->currentIndex());
    ui->comboBox_create_auto_dimension_edge_type_default->setCurrentIndex(index);
}

void SettingGuiCtrl::saveAutoDimensionEdgeType()
{
    int index = ui->comboBox_create_auto_dimension_edge_type_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_EDGE_TYPE, index);
}

void SettingGuiCtrl::readAutoDimensionDispName()
{
    int disp = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_NAME,
                                     ui->checkBox_create_auto_dimension_disp_name_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_disp_name_default->setChecked(disp != 0);
}

void SettingGuiCtrl::saveAutoDimensionDispName()
{
    int disp = ui->checkBox_create_auto_dimension_disp_name_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_NAME, disp);
}

void SettingGuiCtrl::readAutoDimensionTextDrag()
{
    int text_drag = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_TEXT_DRAG,
                                          ui->checkBox_create_auto_dimension_text_drag_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_text_drag_default->setChecked(text_drag != 0);
}

void SettingGuiCtrl::saveAutoDimensionTextDrag()
{
    int text_drag = ui->checkBox_create_auto_dimension_text_drag_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_TEXT_DRAG, text_drag);
}

void SettingGuiCtrl::readAutoDimensionDispUnit()
{
    int text_drag = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_UNIT,
                                          ui->checkBox_create_auto_dimension_unit_disp_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_unit_disp_default->setChecked(text_drag != 0);
}

void SettingGuiCtrl::saveAutoDimensionDispUnit()
{
    int text_drag = ui->checkBox_create_auto_dimension_unit_disp_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_UNIT, text_drag);
}

void SettingGuiCtrl::readAutoDimensionDispMat()
{
    int disp_mat = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_MAT,
                                         ui->checkBox_create_auto_dimension_disp_material_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_disp_material_default->setChecked(disp_mat != 0);
}

void SettingGuiCtrl::saveAutoDimensionDispMat()
{
    int disp_mat = ui->checkBox_create_auto_dimension_disp_material_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_MAT, disp_mat);
}

void SettingGuiCtrl::readAutoDimensionDispValue()
{
    int disp_val = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_VALUE,
                                         ui->checkBox_create_auto_dimension_disp_value_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_disp_value_default->setChecked(disp_val != 0);
}

void SettingGuiCtrl::saveAutoDimensionDispValue()
{
    int disp_val = ui->checkBox_create_auto_dimension_disp_value_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_DISP_VALUE, disp_val);
}

void SettingGuiCtrl::readAutoDimensionExtend()
{
    int extend = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_EXTEND,
                                       ui->checkBox_create_auto_dimension_extend_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_extend_default->setChecked(extend != 0);
}
void SettingGuiCtrl::saveAutoDimensionExtend()
{
    int extend = ui->checkBox_create_auto_dimension_extend_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_EXTEND, extend);
}

void SettingGuiCtrl::readAutoDimensionContinuous()
{
    int cont = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_CONTINUOUS,
                                     ui->checkBox_create_auto_dimension_continue_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_continue_default->setChecked(cont != 0);
}

void SettingGuiCtrl::saveAutoDimensionContinuous()
{
    int cont = ui->checkBox_create_auto_dimension_continue_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_CONTINUOUS, cont);
}

void SettingGuiCtrl::readAutoDimensionAlongPlane()
{
    int along = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_ALONG_PLANE,
                                      ui->checkBox_create_auto_dimension_along_plane_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_along_plane_default->setChecked(along != 0);
}

void SettingGuiCtrl::saveAutoDimensionAlongPlane()
{
    int along_plane = ui->checkBox_create_auto_dimension_along_plane_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_ALONG_PLANE, along_plane);
}

void SettingGuiCtrl::readAutoDimensionSection()
{
    int sec = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_SECTION,
                                    ui->checkBox_create_auto_dimension_section_default->isChecked() ? 1 : 0);
    ui->checkBox_create_auto_dimension_section_default->setChecked(sec != 0);
}

void SettingGuiCtrl::saveAutoDimensionSection()
{
    int sec = ui->checkBox_create_auto_dimension_section_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_SECTION, sec);
}

void SettingGuiCtrl::readAutoDimensionTextAlign()
{
    int tex = m_setting_ctrl.getInt(SettingCtrl::CONFIG_AUTO_DIMENSION_TEXT_ALIGN,
                                    ui->comboBox_create_auto_dimension_text_align_default->currentIndex());
    ui->comboBox_create_auto_dimension_text_align_default->setCurrentIndex(tex);
}

void SettingGuiCtrl::saveAutoDimensionTextAlign()
{
    int tex = ui->comboBox_create_auto_dimension_text_align_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_AUTO_DIMENSION_TEXT_ALIGN, tex);
}

void SettingGuiCtrl::readVoxelDrawMode()
{
    int mode = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOXEL_DRAW_MODE,
                                     ui->comboBox_voxel_draw_mode_default->currentIndex());
    ui->comboBox_voxel_draw_mode_default->setCurrentIndex(mode);
}

void SettingGuiCtrl::saveVoxelDrawMode()
{
    int mode = ui->comboBox_voxel_draw_mode_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOXEL_DRAW_MODE, mode);
}

void SettingGuiCtrl::readVoxelListDoubleClick()
{
    int mode = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOXEL_LIST_DOUBLE_CLICK,
                                     ui->comboBox_doubleClick_default->currentIndex());
    ui->comboBox_doubleClick_default->setCurrentIndex(mode);
}

void SettingGuiCtrl::saveVoxelListDoubleClick()
{
    int mode = ui->comboBox_doubleClick_default->currentIndex();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOXEL_LIST_DOUBLE_CLICK, mode);
}

void SettingGuiCtrl::readVoxelWireColor()
{
    QColor default_color = m_voxel_wire_color;
    default_color.name(QColor::HexArgb);
    QString hexargb =
        m_setting_ctrl.getString(SettingCtrl::CONFIG_VOXEL_WIRE_COLOR, default_color.name(QColor::HexArgb), true);
    m_voxel_wire_color = QColor(hexargb);
}

void SettingGuiCtrl::saveVoxelWireColor()
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_VOXEL_WIRE_COLOR, m_voxel_wire_color.name(QColor::HexArgb));
}

void SettingGuiCtrl::readVoxelWireColorShape()
{
    int shape = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOXEL_WIRE_COLOR_SHAPE,
                                      ui->radioButton_voxel_wire_color_shape_default->isChecked() ? 1 : 0);
    ui->radioButton_voxel_wire_color_shape_default->setChecked(shape != 0);
}

void SettingGuiCtrl::saveVoxelWireColorShape()
{
    int shape = ui->radioButton_voxel_wire_color_shape_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOXEL_WIRE_COLOR_SHAPE, shape);
}

void SettingGuiCtrl::readVoxelWireWidth()
{
    int lw = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOXEL_WIRE_WIDTH, ui->spinBox_voxel_wire_width_default->value());
    ui->spinBox_voxel_wire_width_default->setValue(lw);
}

void SettingGuiCtrl::saveVoxelWireWidth()
{
    int lw = ui->spinBox_voxel_wire_width_default->value();
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOXEL_WIRE_WIDTH, lw);
}

void SettingGuiCtrl::readVoxelOptBaseColor()
{
    QColor default_color = m_voxel_opt_base_color;
    default_color.name(QColor::HexArgb);
    QString hexargb =
        m_setting_ctrl.getString(SettingCtrl::CONFIG_VOXEL_OPT_BASE_COLOR, default_color.name(QColor::HexArgb), true);
    m_voxel_opt_base_color = QColor(hexargb);
}

void SettingGuiCtrl::saveVoxelOptBaseColor()
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_VOXEL_OPT_BASE_COLOR, m_voxel_opt_base_color.name(QColor::HexArgb));
}

void SettingGuiCtrl::readVoxelOptBaseColorShape()
{
    int shape = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOXEL_OPT_BASE_COLOR_SHAPE,
                                      ui->radioButton_voxel_proj_result_shape_color_default->isChecked() ? 1 : 0);
    ui->radioButton_voxel_proj_result_shape_color_default->setChecked(shape != 0);
}

void SettingGuiCtrl::saveVoxelOptBaseColorShape()
{
    int shape = ui->radioButton_voxel_proj_result_shape_color_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOXEL_OPT_BASE_COLOR_SHAPE, shape);
}

void SettingGuiCtrl::readResultColormapMode()
{
    int mode = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_COLOR_MODE,
                                     ui->rb_result3d_cmode_LOG_default->isChecked() ? 1 : 0);
    /// 暫定 :
    /// ラジオボタン制御はボタン一つではなく、全部考慮が本来必要
    /// これまでのはたまたま動いているだけかもしれない（デフォルト変えたら動作しなそうな箇所あり）
    if (mode != 0) {
        ui->rb_result3d_cmode_LOG_default->setChecked(true);
    }
    else {
        ui->rb_result3d_cmode_LINEAR_default->setChecked(true);
    }
}

void SettingGuiCtrl::saveResultColormapMode()
{
    int mode = ui->rb_result3d_cmode_LOG_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_COLOR_MODE, mode);
}

void SettingGuiCtrl::readResultStyle()
{
    auto style = m_setting_ctrl.getString(SettingCtrl::CONFIG_RESULT_STYLE,
                                          ui->comboBox_result3d_custom_setting_default->currentText());

    QStandardItemModel* model = (QStandardItemModel*)ui->comboBox_result3d_custom_setting_default->model();
    for (int ic = 0; ic < ui->comboBox_result3d_custom_setting_default->count(); ++ic) {
        auto color_item = model->item(ic);
        if (color_item && (color_item->flags() & Qt::NoItemFlags)) {
            continue;
        }

        auto text = ui->comboBox_result3d_custom_setting_default->itemText(ic);
        if (text.compare(style, Qt::CaseInsensitive) == 0) {
            ui->comboBox_result3d_custom_setting_default->setCurrentIndex(ic);
            return;
        }
    }
}

void SettingGuiCtrl::saveResultStyle()
{
    auto style = ui->comboBox_result3d_custom_setting_default->currentText();
    m_setting_ctrl.setString(SettingCtrl::CONFIG_RESULT_STYLE, style);
}

void SettingGuiCtrl::readResultAutoColorbar()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_AUTO_COLORBAR,
                                        ui->checkBox_result3d_display_in_view_auto_default->isChecked() ? 1 : 0);
    ui->checkBox_result3d_display_in_view_auto_default->setChecked(checked != 0);
}

void SettingGuiCtrl::saveResultAutoColorbar()
{
    int check = ui->checkBox_result3d_display_in_view_auto_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_AUTO_COLORBAR, check);
}

void SettingGuiCtrl::readResultDisplayPriority()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_DISPLAY_PRIORITY,
                                        ui->checkBox_result3d_display_priority_default->isChecked() ? 1 : 0);
    ui->checkBox_result3d_display_priority_default->setChecked(checked != 0);
}

void SettingGuiCtrl::saveResultDisplayPriority()
{
    int check = ui->checkBox_result3d_display_priority_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_DISPLAY_PRIORITY, check);
}

void SettingGuiCtrl::readResultApplyColorMinMax()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_APPLY_COLOR_VALUE,
                                        ui->checkBox_result3d_apply_color_minmax_default->isChecked() ? 1 : 0);
    ui->checkBox_result3d_apply_color_minmax_default->setChecked(checked != 0);
}

void SettingGuiCtrl::saveResultApplyColorMinMax()
{
    int check = ui->checkBox_result3d_apply_color_minmax_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_APPLY_COLOR_VALUE, check);
}

void SettingGuiCtrl::readResultShowInformationOnClick()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_SHOW_INFO_CLICK,
                                        ui->checkBox_result3d_show_information_on_click_default->isChecked() ? 1 : 0);
    ui->checkBox_result3d_show_information_on_click_default->setChecked(checked != 0);
}

void SettingGuiCtrl::saveResultShowInformationOnClick()
{
    int check = ui->checkBox_result3d_show_information_on_click_default->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_SHOW_INFO_CLICK, check);
}

void SettingGuiCtrl::readResultShowInformationOnClickMSec()
{
    double value = m_setting_ctrl.getDouble(SettingCtrl::CONFIG_RESULT_SHOW_INFO_CLICK_MSEC,
                                            ui->doubleSpinBox_result3d_show_information_on_click_default->value());
    ui->doubleSpinBox_result3d_show_information_on_click_default->setValue(value);
}

void SettingGuiCtrl::saveResultShowInformationOnClickMSec()
{
    double value = ui->doubleSpinBox_result3d_show_information_on_click_default->value();
    m_setting_ctrl.setDouble(SettingCtrl::CONFIG_RESULT_SHOW_INFO_CLICK_MSEC, value);
}

void SettingGuiCtrl::readCPUPriority()
{
    QString priority = m_setting_ctrl.getString(SettingCtrl::CONFIG_PERFORMANCE_CPU_PRIORITY,
                                                ui->comboBox_cpu_priority->currentText());
    ui->comboBox_cpu_priority->setCurrentText(priority);
    if (ui->comboBox_cpu_priority->currentText() != priority) {    /// 不正考慮
        ui->comboBox_cpu_priority->setCurrentText("Normal");
    }
}

void SettingGuiCtrl::saveCPUPriority()
{
    QString priority = ui->comboBox_cpu_priority->currentText();
    m_setting_ctrl.setString(SettingCtrl::CONFIG_PERFORMANCE_CPU_PRIORITY, priority);
}

void SettingGuiCtrl::applyCPUPriority()
{
#ifdef _WIN32
    QString priority = ui->comboBox_cpu_priority->currentText();

    DWORD priority_class = NORMAL_PRIORITY_CLASS;
    if (priority == "Realtime") {
        priority_class = REALTIME_PRIORITY_CLASS;
    }
    else if (priority == "High") {
        priority_class = HIGH_PRIORITY_CLASS;
    }
    else if (priority == "Above Normal") {
        priority_class = ABOVE_NORMAL_PRIORITY_CLASS;
    }
    else if (priority == "Normal") {
        priority_class = NORMAL_PRIORITY_CLASS;
    }
    else if (priority == "Below Normal") {
        priority_class = BELOW_NORMAL_PRIORITY_CLASS;
    }
    else if (priority == "Low") {
        priority_class = IDLE_PRIORITY_CLASS;
    }
    SetPriorityClass(GetCurrentProcess(), priority_class);
#endif
}

bool SettingGuiCtrl::optMemoryLimitDefault()
{
    return false;
}

bool SettingGuiCtrl::readOptMemoryLimit()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_PERFORMANCE_OPT_LIMIT, optMemoryLimitDefault() ? 1 : 0);
    return checked != 0;
}

void SettingGuiCtrl::saveOptMemoryLimit(bool limit)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_PERFORMANCE_OPT_LIMIT, limit ? 1 : 0);
}

double SettingGuiCtrl::optMemorySizeDefault()
{
    return 4.0;
}

double SettingGuiCtrl::readOptMemorySize()
{
    double size = m_setting_ctrl.getDouble(SettingCtrl::CONFIG_PERFORMANCE_OPT_SIZE, optMemorySizeDefault());
    return size;
}

void SettingGuiCtrl::saveOptMemorySize(double size)
{
    m_setting_ctrl.setDouble(SettingCtrl::CONFIG_PERFORMANCE_OPT_SIZE, size);
}

int SettingGuiCtrl::optMemoryUnitDefault()
{
    return 1;
}

int SettingGuiCtrl::readOptMemoryUnit()
{
    const auto& unit =
        m_setting_ctrl.getString(SettingCtrl::CONFIG_PERFORMANCE_OPT_UNIT, optMemoryUnitDefault() ? "GB" : "MB");
    if (unit.compare("MB", Qt::CaseInsensitive) == 0) {
        return 0;
    }
    else {
        return 1;
    }
}

void SettingGuiCtrl::saveOptMemoryUnit(int unit)
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_PERFORMANCE_OPT_UNIT, unit == 0 ? "MB" : "GB");
}

int SettingGuiCtrl::optMemoryMethodDefault()
{
    return 0;
}

int SettingGuiCtrl::readOptMemoryMethod()
{
    int size = m_setting_ctrl.getInt(SettingCtrl::CONFIG_PERFORMANCE_OPT_METHOD, optMemoryMethodDefault());
    return size;
}

void SettingGuiCtrl::saveOptMemoryMethod(int method)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_PERFORMANCE_OPT_METHOD, method);
}

bool SettingGuiCtrl::optMemoryWarnDefault()
{
    return true;
}

bool SettingGuiCtrl::readOptMemoryWarn()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_PERFORMANCE_OPT_WARN, optMemoryWarnDefault() ? 1 : 0);
    return checked != 0;
}

void SettingGuiCtrl::saveOptMemoryWarn(bool warn)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_PERFORMANCE_OPT_WARN, warn ? 1 : 0);
}

void SettingGuiCtrl::readOpenAllOp2Open()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_FILE_LOAD_OP2_OPEN_ALL,
                                        ui->checkBox_open_all_op2_open->isChecked() ? 1 : 0);
    ui->checkBox_open_all_op2_open->setChecked(checked);
}

void SettingGuiCtrl::saveOpenAllOp2Open()
{
    int checked = ui->checkBox_open_all_op2_open->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_FILE_LOAD_OP2_OPEN_ALL, checked);
}

bool SettingGuiCtrl::isOpenAllOp2Open()
{
    if (ui->checkBox_open_all_op2_open->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

void SettingGuiCtrl::readDragDropOp2Open()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_FILE_LOAD_OP2_DROP,
                                        ui->checkBox_drag_and_drop_op2_open->isChecked() ? 1 : 0);
    ui->checkBox_drag_and_drop_op2_open->setChecked(checked);
}

void SettingGuiCtrl::saveDragDropOp2Open()
{
    int checked = ui->checkBox_drag_and_drop_op2_open->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_FILE_LOAD_OP2_DROP, checked);
}

bool SettingGuiCtrl::isDragDropOp2Open()
{
    if (ui->checkBox_drag_and_drop_op2_open->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

void SettingGuiCtrl::readDragDropFolderOpen()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_FILE_LOAD_FOLDER_OPEN,
                                        ui->checkBox_drag_and_drop_folder_open->isChecked() ? 1 : 0);
    ui->checkBox_drag_and_drop_folder_open->setChecked(checked);
}

void SettingGuiCtrl::saveDragDropFolderOpen()
{
    int checked = ui->checkBox_drag_and_drop_folder_open->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_FILE_LOAD_FOLDER_OPEN, checked);
}

bool SettingGuiCtrl::isDragDropFolderOpen()
{
    if (ui->checkBox_drag_and_drop_folder_open->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

void SettingGuiCtrl::readOp2HideArraySetting()
{
    int checked = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_HIDE_OP2_ARRAY_SETTING,
                                        ui->checkBox_op2_load_dsection_dialog_hide->isChecked() ? 1 : 0);
    ui->checkBox_op2_load_dsection_dialog_hide->setChecked(checked);
}

void SettingGuiCtrl::saveOp2HideArraySetting()
{
    int checked = ui->checkBox_op2_load_dsection_dialog_hide->isChecked() ? 1 : 0;
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_HIDE_OP2_ARRAY_SETTING, checked);
}

void SettingGuiCtrl::saveResultOutputImageFormat(int format)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_FORMAT, format);
}

int SettingGuiCtrl::readResultOutputImageFormat()
{
    int format = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_FORMAT, 0);
    return format;
}

void SettingGuiCtrl::saveResultOutputImageSizeType(int type)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_TYPE, type);
}

int SettingGuiCtrl::readResultOutputImageSizeType()
{
    int type = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_TYPE, 0);
    return type;
}

void SettingGuiCtrl::saveResultOutputImageSizeRatio(float ratio)
{
    m_setting_ctrl.setDouble(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_RATIO, ratio);
}

float SettingGuiCtrl::readResultOutputImageSizeRatio()
{
    double ratio = m_setting_ctrl.getDouble(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_RATIO, 2.0);
    return (float)ratio;
}

void SettingGuiCtrl::saveResultOutputImageSizeW(int w)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_W, w);
}

int SettingGuiCtrl::readResultOutputImageSizeW()
{
    int w = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_W, 1);
    return w;
}

void SettingGuiCtrl::saveResultOutputImageSizeH(int h)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_H, h);
}

int SettingGuiCtrl::readResultOutputImageSizeH()
{
    int h = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SIZE_H, 1);
    return h;
}

void SettingGuiCtrl::saveResultOutputImageColorvalType(int type)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_TYPE, type);
}

int SettingGuiCtrl::readResultOutputImageColorvalType()
{
    int type = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_TYPE, 0);
    return type;
}

void SettingGuiCtrl::saveResultOutputImageColorvalMin(const QString& min_value)
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_MIN, min_value);
}

QString SettingGuiCtrl::readResultOutputImageColorvalMin()
{
    const auto& min_value = m_setting_ctrl.getString(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_MIN, "");
    return min_value;
}

void SettingGuiCtrl::saveResultOutputImageColorvalMax(const QString& max_value)
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_MAX, max_value);
}

QString SettingGuiCtrl::readResultOutputImageColorvalMax()
{
    const auto& max_value = m_setting_ctrl.getString(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_MAX, "");
    return max_value;
}

void SettingGuiCtrl::saveResultOutputImageColorbar(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORBAR, check ? 1 : 0);
}

bool SettingGuiCtrl::readResultOutputImageColorbar()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_COLORBAR, 0);
    return check != 0;
}

void SettingGuiCtrl::saveResultOutputImageSpecifyRange(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SPECIFY_RANGE, check ? 1 : 0);
}

bool SettingGuiCtrl::readResultOutputImageSpecifyRange()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_SPECIFY_RANGE, 0);
    return check != 0;
}

void SettingGuiCtrl::saveResultOutputImageDispVox(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_DISP_VOX, check ? 1 : 0);
}

bool SettingGuiCtrl::readResultOutputImageDispVox()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_DISP_VOX, 0);
    return check != 0;
}

void SettingGuiCtrl::saveResultOutputImageBackgroundChange(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_CHANGE, check ? 1 : 0);
}

bool SettingGuiCtrl::readResultOutputImageBackgroundChange()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_CHANGE, 0);
    return check != 0;
}

void SettingGuiCtrl::saveResultOutputImageBackgroundColor(QColor color)
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_COLOR, color.name(QColor::HexArgb));
}

QColor SettingGuiCtrl::readResultOutputImageBackgroundColor()
{
    QColor default_color(Qt::white);
    default_color.name(QColor::HexArgb);
    QString hexargb = m_setting_ctrl.getString(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_COLOR,
                                               default_color.name(QColor::HexArgb), true);
    return QColor(hexargb);
}

void SettingGuiCtrl::saveResultOutputImageBackgroundGrad(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_GRAD, check ? 1 : 0);
}

bool SettingGuiCtrl::readResultOutputImageBackgroundGrad()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_GRAD, 0);
    return check != 0;
}

void SettingGuiCtrl::initResultOutputImage()
{
    m_setting_ctrl.remove(SettingCtrl::RESULT_OUTPUT_IMAGE);    /// セクションごと削除
}

void SettingGuiCtrl::saveVoxOutputImageFormat(int format)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_FORMAT, format);
}

int SettingGuiCtrl::readVoxOutputImageFormat()
{
    int format = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_FORMAT, 0);
    return format;
}

void SettingGuiCtrl::saveVoxOutputImageSizeType(int type)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_TYPE, type);
}

int SettingGuiCtrl::readVoxOutputImageSizeType()
{
    int type = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_TYPE, 0);
    return type;
}

void SettingGuiCtrl::saveVoxOutputImageSizeRatio(float ratio)
{
    m_setting_ctrl.setDouble(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_RATIO, ratio);
}

float SettingGuiCtrl::readVoxOutputImageSizeRatio()
{
    double ratio = m_setting_ctrl.getDouble(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_RATIO, 2.0);
    return static_cast<float>(ratio);
}

void SettingGuiCtrl::saveVoxOutputImageSizeW(int w)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_W, w);
}

int SettingGuiCtrl::readVoxOutputImageSizeW()
{
    int w = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_W, 1);
    return w;
}

void SettingGuiCtrl::saveVoxOutputImageSizeH(int h)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_H, h);
}

int SettingGuiCtrl::readVoxOutputImageSizeH()
{
    int h = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_SIZE_H, 1);
    return h;
}

void SettingGuiCtrl::saveVoxOutputImageDispVox(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_DISP_VOX, check ? 1 : 0);
}

bool SettingGuiCtrl::readVoxOutputImageDispVox()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_DISP_VOX, 1);
    return check != 0;
}

void SettingGuiCtrl::saveVoxOutputImageFitDispVox(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_FIT_DISP_VOX, check ? 1 : 0);
}

bool SettingGuiCtrl::readVoxOutputImageFitDispVox()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_FIT_DISP_VOX, 0);
    return check != 0;
}

void SettingGuiCtrl::saveVoxOutputImageBackgroundChange(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_CHANGE, check ? 1 : 0);
}

bool SettingGuiCtrl::readVoxOutputImageBackgroundChange()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_CHANGE, 0);
    return check != 0;
}

void SettingGuiCtrl::saveVoxOutputImageBackgroundColor(QColor color)
{
    m_setting_ctrl.setString(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_COLOR, color.name(QColor::HexArgb));
}

QColor SettingGuiCtrl::readVoxOutputImageBackgroundColor()
{
    QColor  default_color(Qt::white);
    QString hexargb = m_setting_ctrl.getString(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_COLOR,
                                               default_color.name(QColor::HexArgb), true);
    return QColor(hexargb);
}

void SettingGuiCtrl::saveVoxOutputImageBackgroundGrad(bool check)
{
    m_setting_ctrl.setInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_GRAD, check ? 1 : 0);
}

bool SettingGuiCtrl::readVoxOutputImageBackgroundGrad()
{
    int check = m_setting_ctrl.getInt(SettingCtrl::CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_GRAD, 0);
    return check != 0;
}

void SettingGuiCtrl::initVoxOutputImage()
{
    m_setting_ctrl.remove(SettingCtrl::VOX_OUTPUT_IMAGE);    /// セクションごと削除
}

void SettingGuiCtrl::saveMainWindowInitState()
{
    saveMainWindowState((QMainWindow*)m_parent->parentWidget(), *m_init_geometry_setting);
}

void SettingGuiCtrl::saveMainWindowState()
{
    SettingsWrapper geometry_setting(SettingsWrapper::Mode::UseQSettings,
                                     SettingCtrl::documentPath() + "/Eyeris3DViewer_GUI_layout.ini");
    saveMainWindowState((QMainWindow*)m_parent->parentWidget(), geometry_setting);
}

void SettingGuiCtrl::readMainWindowState()
{
    QFile geometry_setting(SettingCtrl::documentPath() + "/Eyeris3DViewer_GUI_layout.ini");
    if (geometry_setting.exists()) {
        SettingsWrapper geometry_setting(SettingsWrapper::Mode::UseQSettings,
                                         SettingCtrl::documentPath() + "/Eyeris3DViewer_GUI_layout.ini");
        readMainWindowState((QMainWindow*)m_parent->parentWidget(), geometry_setting);
    }
    else {
        readMainWindowState((QMainWindow*)m_parent->parentWidget(), *m_init_geometry_setting);
    }
}

void SettingGuiCtrl::resetMainWindowState()
{
    QFile geometry_setting(SettingCtrl::documentPath() + "/Eyeris3DViewer_GUI_layout.ini");
    if (geometry_setting.exists()) {
        geometry_setting.remove();
    }
}

void SettingGuiCtrl::saveMainWindowState(QMainWindow* mainWindow, SettingsWrapper& settings)
{
    QString version = "1.0";

    mainWindow->setUpdatesEnabled(false);

    bool maximized = mainWindow->isMaximized();
    settings.setValue(version + "/mainwindow_maximized", maximized ? "1" : "0");    /// とりあえず保存しておく
    // if (maximized) {
    //     mainWindow->showNormal();
    //     qApp->processEvents();
    // }

    QSet<QDockWidget*> current_docks = m_parent->tabActiveWidgets();    /// カレント（全面表示）が変わってしまうので保持
    auto dock_widget = m_parent->dockWidgets();
    for (auto& dock : dock_widget) {
        int flag = dock->isVisible() ? 1 : 0;
        if (current_docks.contains(dock)) {
            flag = 2;
        }
        settings.setValue(version + "/" + dock->objectName(), flag);
    }
    for (auto& dock : dock_widget) {
        dock->hide();
    }
    qApp->processEvents();

    settings.setValue(version + "/mainwindow_geometry", mainWindow->saveGeometry());
    settings.setValue(version + "/mainwindow_state", mainWindow->saveState());

    for (auto& dock : dock_widget) {
        const auto& visible = settings.value(version + "/" + dock->objectName(), "").toString();
        if (visible == "1" || visible == "2") {
            dock->show();
        }
    }

    for (auto& dock : dock_widget) {
        const auto& visible = settings.value(version + "/" + dock->objectName(), "").toString();
        if (visible == "2") {
            dock->raise();
        }
    }

    mainWindow->setUpdatesEnabled(true);

    // if (maximized) {
    //     mainWindow->showMaximized();
    // }
}

void SettingGuiCtrl::readMainWindowState(QMainWindow* mainWindow, SettingsWrapper& settings)
{
    QString version = "1.0";

    // mainWindow->setUpdatesEnabled(false);

    const auto& geometry = settings.value(version + "/mainwindow_geometry", "").toByteArray();
    if (!geometry.isEmpty()) {
        mainWindow->restoreGeometry(geometry);
    }
    const auto& state = settings.value(version + "/mainwindow_state", "").toByteArray();
    if (!state.isEmpty()) {
        mainWindow->restoreState(state);
    }

    QMap<QDockWidget*, QString> dockwidget_state;
    auto                        dock_widget = m_parent->dockWidgets();
    for (auto& dock : dock_widget) {
        const auto& visible = settings.value(version + "/" + dock->objectName(), "").toString();
        if (!visible.isEmpty()) {
            dockwidget_state[dock] = visible;
        }
    }

    // const auto& maximized = geometry_setting.value(version + "/mainwindow_maximized", "").toString();

    if (!dockwidget_state.isEmpty() /*|| maximized == "1"*/) {
        QTimer::singleShot(0, [this, mainWindow, dockwidget_state /*,maximized*/]() {
            auto dock_widget = m_parent->dockWidgets();
            for (auto& dock : dock_widget) {
                const auto& visible = dockwidget_state.value(dock, "1");
                if (visible == "1" || visible == "2") {
                    dock->show();
                }
            }
            for (auto& dock : dock_widget) {
                const auto& visible = dockwidget_state.value(dock, "1");
                if (visible == "2") {
                    dock->raise();
                }
            }

            // if (maximized == "1") {
            //     QTimer::singleShot(0, [mainWindow]() { mainWindow->showMaximized(); });
            // }

            // mainWindow->setUpdatesEnabled(true);
        });
    }
    else {
        // mainWindow->setUpdatesEnabled(true);
    }
}
