#include "ClippingCtrl.h"
#include "DimensionCtrl.h"
#include "MyOpenGLWidget.h"
#include "RangeSlider.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "Scene/MultiDimension.h"
#include "Scene/SceneView.h"
#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"
#include "Utils/Clipping.h"

#include "Vox3DForm.h"
#include "ui_Vox3DForm.h"

ClippingCtrl::ClippingCtrl(Vox3DForm* form, Ui::Vox3DForm* ui_)
    : QObject(form)
    , ui(ui_)
    , m_3DForm(form)
    , m_suppress_update_clipping(false)
    , m_step(1.0f)
{
    m_all_box.set(0, 0, 0, 1, 1, 1);

    /// SpinBox設定
    ui->doubleSpinBox_ClipXMin->setEnabled(false);
    ui->doubleSpinBox_ClipXMax->setEnabled(false);
    ui->doubleSpinBox_ClipYMin->setEnabled(false);
    ui->doubleSpinBox_ClipYMax->setEnabled(false);
    ui->doubleSpinBox_ClipZMin->setEnabled(false);
    ui->doubleSpinBox_ClipZMax->setEnabled(false);
    ui->doubleSpinBox_ClipAnyMin->setEnabled(false);
    ui->doubleSpinBox_ClipAnyMax->setEnabled(false);
    ui->horizontalSlider_ClipX->setSpinBox(ui->doubleSpinBox_ClipXMin, ui->doubleSpinBox_ClipXMax);
    ui->horizontalSlider_ClipY->setSpinBox(ui->doubleSpinBox_ClipYMin, ui->doubleSpinBox_ClipYMax);
    ui->horizontalSlider_ClipZ->setSpinBox(ui->doubleSpinBox_ClipZMin, ui->doubleSpinBox_ClipZMax);
    ui->horizontalSlider_ClipAny->setSpinBox(ui->doubleSpinBox_ClipAnyMin, ui->doubleSpinBox_ClipAnyMax);

    /// 処理共通化
    std::vector<std::tuple<QPushButton*, QPushButton*, RangeSlider*, QCheckBox*>> clipping_controls = {
        {ui->pushButton_ClipXMin,   ui->pushButton_ClipXMax,   ui->horizontalSlider_ClipX,   ui->checkBox_ClipX2d  },
        {ui->pushButton_ClipYMin,   ui->pushButton_ClipYMax,   ui->horizontalSlider_ClipY,   ui->checkBox_ClipY2d  },
        {ui->pushButton_ClipZMin,   ui->pushButton_ClipZMax,   ui->horizontalSlider_ClipZ,   ui->checkBox_ClipZ2d  },
        {ui->pushButton_ClipAnyMin, ui->pushButton_ClipAnyMax, ui->horizontalSlider_ClipAny, ui->checkBox_ClipAny2d},
    };

    for (const auto& control : clipping_controls) {
        QPushButton* lower_button  = std::get<0>(control);
        QPushButton* upper_button  = std::get<1>(control);
        RangeSlider* slider        = std::get<2>(control);
        QCheckBox*   clip_2d_check = std::get<3>(control);

        /// 有効無効ボタン設定
        connect(lower_button, &QPushButton::clicked, this,
                [this, upper_button, lower_button, slider, clip_2d_check](bool) {
                    if (lower_button->isChecked()) {
                        if (ui->checkBox_clip_2d_mode->isChecked()) {
                            reset(true);
                            lower_button->blockSignals(true);
                            lower_button->setChecked(true);
                            lower_button->blockSignals(false);
                        }
                        else if (clip_2d_check->isChecked()) {
                            if (upper_button->isChecked()) {
                                upper_button->setChecked(false);
                                slider->setEnableUpper(false);
                            }
                        }

                        if (ui->pushButton_ClipAnyMin == lower_button) {
                            if (!ui->pushButton_ClipAnyMax->isChecked()) {
                                setRangeAny(true);
                            }
                        }
                        slider->setEnableLower(true);
                    }
                    else {
                        slider->setEnableLower(false);
                    }
                    updateClipping();
                });
        connect(upper_button, &QPushButton::clicked, this,
                [this, upper_button, lower_button, slider, clip_2d_check](bool) {
                    if (upper_button->isChecked()) {
                        if (ui->checkBox_clip_2d_mode->isChecked()) {
                            reset(true);
                            upper_button->blockSignals(true);
                            upper_button->setChecked(true);
                            upper_button->blockSignals(false);
                        }
                        else if (clip_2d_check->isChecked()) {
                            if (lower_button->isChecked()) {
                                lower_button->setChecked(false);
                                slider->setEnableLower(false);
                            }
                        }

                        if (ui->pushButton_ClipAnyMax == upper_button) {
                            if (!ui->pushButton_ClipAnyMin->isChecked()) {
                                setRangeAny(true);
                            }
                        }
                        slider->setEnableUpper(true);
                    }
                    else {
                        slider->setEnableUpper(false);
                    }
                    updateClipping();
                });

        /// 2Dモードボタン
        connect(clip_2d_check, &QCheckBox::clicked, this,
                [this, upper_button, lower_button, slider, clip_2d_check, clipping_controls](bool) {
                    SuppressUpdateClipping suppress_clipping(this);
                    if (clip_2d_check->isChecked()) {
                        for (const auto& control : clipping_controls) {
                            QCheckBox* clip_2d_check_other = std::get<3>(control);
                            if (clip_2d_check_other != clip_2d_check) {
                                clip_2d_check_other->blockSignals(true);
                                clip_2d_check_other->setChecked(false);
                                clip_2d_check_other->blockSignals(false);
                            }
                        }
                        if (upper_button->isChecked() && lower_button->isChecked()) {
                            if (slider->isActiveUpper()) {
                                lower_button->setChecked(false);
                                slider->setEnableLower(false);
                            }
                            else {
                                upper_button->setChecked(false);
                                slider->setEnableUpper(false);
                            }
                        }
                    }
                    suppress_clipping.reset();
                    updateClipping(true);
                });

        /// スライダー設定
        connect(slider, &RangeSlider::lowerValueChanged, this, [this]() { updateClipping(); });
        connect(slider, &RangeSlider::upperValueChanged, this, [this]() { updateClipping(); });
    }

    connect(ui->checkBox_clip_2d_mode, &QCheckBox::toggled, this, [this, clipping_controls]() {
        SuppressUpdateClipping suppress_clipping(this);
        if (ui->checkBox_clip_2d_mode->isChecked()) {
            int active_2d_index = -1;
            int active_index    = -1;
            for (int ic = 0; ic < (int)clipping_controls.size(); ++ic) {
                const auto&  control       = clipping_controls[ic];
                QPushButton* lower_button  = std::get<0>(control);
                QPushButton* upper_button  = std::get<1>(control);
                QCheckBox*   clip_2d_check = std::get<3>(control);

                if (lower_button->isChecked() || upper_button->isChecked()) {
                    if (clip_2d_check->isChecked()) {
                        if (active_2d_index < 0) {
                            active_2d_index = ic;
                        }
                    }
                    if (active_index < 0) {
                        active_index = ic;
                    }
                }
                if (active_index >= 0 && active_2d_index >= 0) {
                    break;
                }
            }
            if (active_2d_index >= 0) {
                active_index = active_2d_index;
            }
            if (active_index >= 0) {
                const auto&  control      = clipping_controls[active_index];
                QPushButton* lower_button = std::get<0>(control);
                QPushButton* upper_button = std::get<1>(control);
                RangeSlider* slider       = std::get<2>(control);

                if (lower_button->isChecked() && upper_button->isChecked()) {
                    if (slider->isActiveUpper()) {
                        reset(true);
                        upper_button->blockSignals(true);
                        upper_button->setChecked(true);
                        upper_button->blockSignals(false);
                        slider->setEnableUpper(true);
                    }
                    else {
                        reset(true);
                        lower_button->blockSignals(true);
                        lower_button->setChecked(true);
                        lower_button->blockSignals(false);
                        slider->setEnableLower(true);
                    }
                }
                else if (lower_button->isChecked()) {
                    reset(true);
                    lower_button->blockSignals(true);
                    lower_button->setChecked(true);
                    lower_button->blockSignals(false);
                    slider->setEnableLower(true);
                }
                else if (upper_button->isChecked()) {
                    reset(true);
                    upper_button->blockSignals(true);
                    upper_button->setChecked(true);
                    upper_button->blockSignals(false);
                    slider->setEnableUpper(true);
                }
            }

            ui->checkBox_ClipX2d->setEnabled(false);
            ui->checkBox_ClipY2d->setEnabled(false);
            ui->checkBox_ClipZ2d->setEnabled(false);
            ui->checkBox_ClipAny2d->setEnabled(false);
            // ui->pushButton_ClipX2d->setChecked(false);
            // ui->pushButton_ClipY2d->setChecked(false);
            // ui->pushButton_ClipZ2d->setChecked(false);
            // ui->pushButton_ClipAny2d->setChecked(false);
        }
        else {
            ui->checkBox_ClipX2d->setEnabled(true);
            ui->checkBox_ClipY2d->setEnabled(true);
            ui->checkBox_ClipZ2d->setEnabled(true);
            ui->checkBox_ClipAny2d->setEnabled(true);
        }
        suppress_clipping.reset();
        updateClipping(true);
    });

    connect(ui->pushButton_ClipXMinView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->rightView(); });
    connect(ui->pushButton_ClipXMaxView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->leftView(); });
    connect(ui->pushButton_ClipYMinView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->frontView(); });
    connect(ui->pushButton_ClipYMaxView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->backView(); });
    connect(ui->pushButton_ClipZMinView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->bottomView(); });
    connect(ui->pushButton_ClipZMaxView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->topView(); });
    connect(ui->pushButton_ClipAnyMinView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->dirView(m_planes[PlaneAnyMin].m_plane.normal()); });
    connect(ui->pushButton_ClipAnyMaxView, &QPushButton::clicked, this,
            [this](bool /*checked*/) { ui->obj3dViewer->dirView(m_planes[PlaneAnyMax].m_plane.normal()); });

    connect(ui->pushButton_ClipClear, &QPushButton::clicked, this, [this](bool /*checked*/) { reset(true); });

    /// Resultの方のクリア
    connect(ui->pushButton_result3d_clear_clipping, &QPushButton::clicked, this,
            [this](bool /*checked*/) { reset(true); });

    /// Any Init
    ui->radioButton_ClipAnyDirNorm->setChecked(false);
    ui->label_ClipAny_DirX->setText("Θ");
    ui->label_ClipAny_DirY->setText("φ");
    ui->radioButton_ClipAnyDirAngle->setChecked(true);
    ui->label_ClipAnyDirAngle->setVisible(true);
    ui->label_ClipAny_DirZ->setVisible(false);
    ui->doubleSpinBox_ClipAnyDirZ->setVisible(false);

    ui->doubleSpinBox_ClipAnyDirX->setValue(90.0);
    ui->doubleSpinBox_ClipAnyDirY->setValue(45.0);
    ui->doubleSpinBox_ClipAnyDirX->setDecimals(2);
    ui->doubleSpinBox_ClipAnyDirY->setDecimals(2);
    ui->doubleSpinBox_ClipAnyDirX->setSingleStep(1);
    ui->doubleSpinBox_ClipAnyDirY->setSingleStep(1);

    /// 暫定: オプションを切り替えたとき、GUIがちらつく（サイズが変わる）ので、それを防止
    ui->doubleSpinBox_ClipAnyPosX->setFixedSize(ui->doubleSpinBox_ClipAnyPosX->sizeHint());
    ui->doubleSpinBox_ClipAnyPosY->setFixedSize(ui->doubleSpinBox_ClipAnyPosY->sizeHint());
    ui->doubleSpinBox_ClipAnyPosZ->setFixedSize(ui->doubleSpinBox_ClipAnyPosZ->sizeHint());

    connect(ui->radioButton_ClipAnyDirNorm, &QRadioButton::toggled, this, &ClippingCtrl::changeAnyDirAngle);

    connect(ui->doubleSpinBox_ClipAnyDirX, &QDoubleSpinBox::valueChanged, this, &ClippingCtrl::anyValueChanged);
    connect(ui->doubleSpinBox_ClipAnyDirY, &QDoubleSpinBox::valueChanged, this, &ClippingCtrl::anyValueChanged);
    connect(ui->doubleSpinBox_ClipAnyDirZ, &QDoubleSpinBox::valueChanged, this, &ClippingCtrl::anyValueChanged);
    connect(ui->doubleSpinBox_ClipAnyPosX, &QDoubleSpinBox::valueChanged, this, &ClippingCtrl::anyValueChanged);
    connect(ui->doubleSpinBox_ClipAnyPosY, &QDoubleSpinBox::valueChanged, this, &ClippingCtrl::anyValueChanged);
    connect(ui->doubleSpinBox_ClipAnyPosZ, &QDoubleSpinBox::valueChanged, this, &ClippingCtrl::anyValueChanged);

    /// スケール
    ui->radioButton_ClipScale_Real->setChecked(true);
    ui->radioButton_ClipScale_Cell->setChecked(false);
    connect(ui->radioButton_ClipScale_Real, &QRadioButton::toggled, this, &ClippingCtrl::changeScaleRealCell);

    /// プリセット
    initAnyPreset();
    // connect(ui->comboBox_any_clip_preset, &QComboBox::currentIndexChanged, this, &ClippingCtrl::setAnyPreset);
    connect(ui->comboBox_any_clip_preset, &QComboBox::activated, this, &ClippingCtrl::setAnyPreset);

    /// 初期化
    reset(false);
}

void ClippingCtrl::reset(bool keep_setting_value)
{
    Clipping::deleteWorkMemory();

    /// Resultの方のボタンの有効無効設定
    ui->pushButton_result3d_clear_clipping->setEnabled(false);

    /// 処理共通化
    std::vector<std::tuple<QPushButton*, QPushButton*, RangeSlider*>> clipping_controls = {
        {ui->pushButton_ClipXMin,   ui->pushButton_ClipXMax,   ui->horizontalSlider_ClipX  },
        {ui->pushButton_ClipYMin,   ui->pushButton_ClipYMax,   ui->horizontalSlider_ClipY  },
        {ui->pushButton_ClipZMin,   ui->pushButton_ClipZMax,   ui->horizontalSlider_ClipZ  },
        {ui->pushButton_ClipAnyMin, ui->pushButton_ClipAnyMax, ui->horizontalSlider_ClipAny},
    };

    SuppressUpdateClipping suppress_clipping(this);

    for (const auto& control : clipping_controls) {
        QPushButton* lower_button = std::get<0>(control);
        QPushButton* upper_button = std::get<1>(control);
        RangeSlider* slider       = std::get<2>(control);
        if (lower_button->isChecked()) lower_button->setChecked(false);
        if (upper_button->isChecked()) upper_button->setChecked(false);
        if (slider->isEnableLower()) slider->setEnableLower(false);
        if (slider->isEnableUpper()) slider->setEnableUpper(false);
    }

    float         step_size;
    float         cell_size = 1.0f;
    BoundingBox3f bbox      = boxAndStepSize(step_size, cell_size, !keep_setting_value);

    float nm_cell_size = (step_size * cell_size * m_disp_unit);
    if (nm_cell_size >= 10) {
        m_lower_upper_diff_min_nm   = 0.1;     /// 0.1nm
        m_lower_upper_diff_min_cell = 0.01;    /// 0.1nm
    }
    else {
        /// 暫定:1nm以下を考慮しない（valid_lengthとかも変える必要ある。全体で考慮）
        m_lower_upper_diff_min_nm   = 0.1;    /// 0.1nm
        m_lower_upper_diff_min_cell = 0.1;    /// 0.1nm
    }

    /// 共通にしない方が見通せる？のでそのまま

    if (!keep_setting_value) {
        if (ui->radioButton_ClipScale_Real->isChecked()) {
            ui->horizontalSlider_ClipX->init(bbox.xMin() * m_disp_unit, bbox.xMax() * m_disp_unit,
                                             step_size * cell_size * m_disp_unit, -1, m_lower_upper_diff_min_nm);
            ui->horizontalSlider_ClipY->init(bbox.yMin() * m_disp_unit, bbox.yMax() * m_disp_unit,
                                             step_size * cell_size * m_disp_unit, -1, m_lower_upper_diff_min_nm);
            ui->horizontalSlider_ClipZ->init(bbox.zMin() * m_disp_unit, bbox.zMax() * m_disp_unit,
                                             step_size * cell_size * m_disp_unit, -1, m_lower_upper_diff_min_nm);
        }
        else {
            ui->horizontalSlider_ClipX->init(bbox.xMin() / step_size, bbox.xMax() / step_size, cell_size, -1,
                                             m_lower_upper_diff_min_cell);
            ui->horizontalSlider_ClipY->init(bbox.yMin() / step_size, bbox.yMax() / step_size, cell_size, -1,
                                             m_lower_upper_diff_min_cell);
            ui->horizontalSlider_ClipZ->init(bbox.zMin() / step_size, bbox.zMax() / step_size, cell_size, -1,
                                             m_lower_upper_diff_min_cell);
        }
        m_step = step_size;

        setRangeAny(keep_setting_value);
    }

    m_planes[PlaneXMin].m_plane.setFromNormalAndPoint(Point3f(1, 0, 0), Point3f(bbox.xMin(), 0, 0));
    m_planes[PlaneXMin].m_enable = false;
    m_planes[PlaneXMax].m_plane.setFromNormalAndPoint(Point3f(-1, 0, 0), Point3f(bbox.xMax(), 0, 0));
    m_planes[PlaneXMax].m_enable = false;

    m_planes[PlaneYMin].m_plane.setFromNormalAndPoint(Point3f(0, 1, 0), Point3f(bbox.yMin(), 0, 0));
    m_planes[PlaneYMin].m_enable = false;
    m_planes[PlaneYMax].m_plane.setFromNormalAndPoint(Point3f(0, -1, 0), Point3f(bbox.yMax(), 0, 0));
    m_planes[PlaneYMax].m_enable = false;

    m_planes[PlaneZMin].m_plane.setFromNormalAndPoint(Point3f(0, 0, 1), Point3f(bbox.zMin(), 0, 0));
    m_planes[PlaneZMin].m_enable = false;
    m_planes[PlaneZMax].m_plane.setFromNormalAndPoint(Point3f(0, 0, -1), Point3f(bbox.zMax(), 0, 0));
    m_planes[PlaneZMax].m_enable = false;

    m_planes[PlaneAnyMin].m_enable = false;
    m_planes[PlaneAnyMax].m_enable = false;

    if (ui->radioButton_ClipScale_Real->isChecked()) {
        ui->doubleSpinBox_ClipAnyPosX->setSingleStep(step_size * cell_size * m_disp_unit);
        ui->doubleSpinBox_ClipAnyPosY->setSingleStep(step_size * cell_size * m_disp_unit);
        ui->doubleSpinBox_ClipAnyPosZ->setSingleStep(step_size * cell_size * m_disp_unit);
    }
    else {
        ui->doubleSpinBox_ClipAnyPosX->setSingleStep(cell_size);
        ui->doubleSpinBox_ClipAnyPosY->setSingleStep(cell_size);
        ui->doubleSpinBox_ClipAnyPosZ->setSingleStep(cell_size);
    }

    suppress_clipping.reset();
    updateClipping();
}

float floorToDigits(float value, int digits)
{
    float factor = std::pow(10.0f, digits);
    return std::floor(value * factor) / factor;
}

float ceilToDigits(float value, int digits)
{
    float factor = std::pow(10.0f, digits);
    return std::ceil(value * factor) / factor;
}

void ClippingCtrl::applyPlanes(const std::vector<Planef>& planes, bool any_min_priority, bool mode_2d)
{
    reset(true);    /// SuppressUpdateClippingより先に実施（クリッピング解除が必要なので）
    SuppressUpdateClipping suppress_clipping(this);
    reset(false);

    double step = 1.0;
    if (!ui->radioButton_ClipScale_Real->isChecked()) {
        step = m_step;
    }
    else {
        step = 1.0f / m_disp_unit;
    }

    // 判定用しきい値
    const float eps = 1e-12f;

    bool    has_any = false;    /// 現状Anyは１方向だけなので
    Point3f any_min_norm;

    for (const auto& plane : planes) {
        Point3f normal = plane.normal();
        Point3f origin = plane.origin();

        /// X Min
        if (m_planes[PlaneXMin].m_plane.normal().distance2(normal) <= eps) {
            ui->pushButton_ClipXMin->setChecked(true);
            emit ui->pushButton_ClipXMin->clicked();
            ui->doubleSpinBox_ClipXMin->setValue(origin.x() / step);
        }
        /// XMax
        else if (m_planes[PlaneXMax].m_plane.normal().distance2(normal) <= eps) {
            ui->pushButton_ClipXMax->setChecked(true);
            emit ui->pushButton_ClipXMax->clicked();
            ui->doubleSpinBox_ClipXMax->setValue(origin.x() / step);
        }
        /// YMin
        else if (m_planes[PlaneYMin].m_plane.normal().distance2(normal) <= eps) {
            ui->pushButton_ClipYMin->setChecked(true);
            emit ui->pushButton_ClipYMin->clicked();
            ui->doubleSpinBox_ClipYMin->setValue(origin.y() / step);
        }
        /// YMax
        else if (m_planes[PlaneYMax].m_plane.normal().distance2(normal) <= eps) {
            ui->pushButton_ClipYMax->setChecked(true);
            emit ui->pushButton_ClipYMax->clicked();
            ui->doubleSpinBox_ClipYMax->setValue(origin.y() / step);
        }
        /// ZMin
        else if (m_planes[PlaneZMin].m_plane.normal().distance2(normal) <= eps) {
            ui->pushButton_ClipZMin->setChecked(true);
            emit ui->pushButton_ClipZMin->clicked();
            ui->doubleSpinBox_ClipZMin->setValue(origin.z() / step);
        }
        /// ZMax
        else if (m_planes[PlaneZMax].m_plane.normal().distance2(normal) <= eps) {
            ui->pushButton_ClipZMax->setChecked(true);
            emit ui->pushButton_ClipZMax->clicked();
            ui->doubleSpinBox_ClipZMax->setValue(origin.z() / step);
        }
        /// Any(初回）
        else if (!has_any) {
            /// AnyのMinとMaxどっちで合わせるか
            if (any_min_priority) {
                any_min_norm = normal;
            }
            else {
                any_min_norm = -normal;
            }

            if (ui->radioButton_ClipAnyDirNorm->isChecked()) {
                ui->doubleSpinBox_ClipAnyDirX->setValue(any_min_norm.x());
                ui->doubleSpinBox_ClipAnyDirY->setValue(any_min_norm.y());
                ui->doubleSpinBox_ClipAnyDirZ->setValue(any_min_norm.z());
            }
            else {
                /// 法線⇒角度
                double theta, phi;
                auto   r = std::sqrt(any_min_norm.x() * any_min_norm.x() + any_min_norm.y() * any_min_norm.y()
                                     + any_min_norm.z() * any_min_norm.z());
                if (r != 0) {
                    theta = std::acos(any_min_norm.z() / r);
                }
                else {
                    theta = 0.0;
                }

                if (any_min_norm.x() == 0.0 && any_min_norm.y() == 0.0) {
                    phi = 0.0;
                }
                else {
                    phi = std::atan2(any_min_norm.y(), any_min_norm.x());
                }
                ui->doubleSpinBox_ClipAnyDirX->setValue(radianToDegree(theta));
                ui->doubleSpinBox_ClipAnyDirY->setValue(radianToDegree(phi));
            }

            /// 範囲指定
            setRangeAny(true);

            if (any_min_priority) {
                ui->pushButton_ClipAnyMin->setChecked(true);
                emit ui->pushButton_ClipAnyMin->clicked();

                /// Any 原点
                float any_org_x = (float)ui->doubleSpinBox_ClipAnyPosX->value();
                float any_org_y = (float)ui->doubleSpinBox_ClipAnyPosY->value();
                float any_org_z = (float)ui->doubleSpinBox_ClipAnyPosZ->value();
                if (!ui->radioButton_ClipScale_Real->isChecked()) {
                    any_org_x *= m_step;
                    any_org_y *= m_step;
                    any_org_z *= m_step;
                }
                else {
                    any_org_x /= m_disp_unit;
                    any_org_y /= m_disp_unit;
                    any_org_z /= m_disp_unit;
                }
                /// valueはPlaneAnyMinで計算
                Planef plane(m_planes[PlaneAnyMin].m_plane.normal(), Point3f(any_org_x, any_org_y, any_org_z));
                float  dist =
                    floorToDigits(plane.distanceToPoint(origin) / step, ui->doubleSpinBox_ClipAnyMin->decimals());
                ui->doubleSpinBox_ClipAnyMin->setValue(dist);
            }
            else {
                ui->pushButton_ClipAnyMax->setChecked(true);
                emit ui->pushButton_ClipAnyMax->clicked();

                /// Any 原点
                float any_org_x = (float)ui->doubleSpinBox_ClipAnyPosX->value();
                float any_org_y = (float)ui->doubleSpinBox_ClipAnyPosY->value();
                float any_org_z = (float)ui->doubleSpinBox_ClipAnyPosZ->value();
                if (!ui->radioButton_ClipScale_Real->isChecked()) {
                    any_org_x *= m_step;
                    any_org_y *= m_step;
                    any_org_z *= m_step;
                }
                else {
                    any_org_x /= m_disp_unit;
                    any_org_y /= m_disp_unit;
                    any_org_z /= m_disp_unit;
                }
                /// valueはPlaneAnyMinで計算
                Planef plane(m_planes[PlaneAnyMin].m_plane.normal(), Point3f(any_org_x, any_org_y, any_org_z));
                float  dist =
                    ceilToDigits(plane.distanceToPoint(origin) / step, ui->doubleSpinBox_ClipAnyMax->decimals());
                ui->doubleSpinBox_ClipAnyMax->setValue(dist);
            }
        }
        /// Any(２回目以降）
        else {
            /// 暫定 -作ったが未検証。サムネイル（現在の状態再現）機能で使うはず。そこで検証

            if (any_min_priority) {
                if (any_min_norm.distance2(-normal) <= eps) {
                    ui->pushButton_ClipAnyMax->setChecked(true);
                    emit ui->pushButton_ClipAnyMax->clicked();

                    /// Any 原点
                    float any_org_x = (float)ui->doubleSpinBox_ClipAnyPosX->value();
                    float any_org_y = (float)ui->doubleSpinBox_ClipAnyPosY->value();
                    float any_org_z = (float)ui->doubleSpinBox_ClipAnyPosZ->value();
                    if (!ui->radioButton_ClipScale_Real->isChecked()) {
                        any_org_x *= m_step;
                        any_org_y *= m_step;
                        any_org_z *= m_step;
                    }
                    else {
                        any_org_x /= m_disp_unit;
                        any_org_y /= m_disp_unit;
                        any_org_z /= m_disp_unit;
                    }
                    /// valueはPlaneAnyMinで計算
                    float dist =
                        ceilToDigits(plane.distanceToPoint(origin) / step, ui->doubleSpinBox_ClipAnyMax->decimals());
                    ui->doubleSpinBox_ClipAnyMax->setValue(dist);
                }
            }
            else {
                if (any_min_norm.distance2(normal) <= eps) {
                    ui->pushButton_ClipAnyMin->setChecked(true);
                    emit ui->pushButton_ClipAnyMin->clicked();

                    /// Any 原点
                    float any_org_x = (float)ui->doubleSpinBox_ClipAnyPosX->value();
                    float any_org_y = (float)ui->doubleSpinBox_ClipAnyPosY->value();
                    float any_org_z = (float)ui->doubleSpinBox_ClipAnyPosZ->value();
                    if (!ui->radioButton_ClipScale_Real->isChecked()) {
                        any_org_x *= m_step;
                        any_org_y *= m_step;
                        any_org_z *= m_step;
                    }
                    else {
                        any_org_x /= m_disp_unit;
                        any_org_y /= m_disp_unit;
                        any_org_z /= m_disp_unit;
                    }
                    /// valueはPlaneAnyMinで計算
                    Planef plane(m_planes[PlaneAnyMin].m_plane.normal(), Point3f(any_org_x, any_org_y, any_org_z));
                    float  dist =
                        floorToDigits(plane.distanceToPoint(origin) / step, ui->doubleSpinBox_ClipAnyMin->decimals());
                    ui->doubleSpinBox_ClipAnyMin->setValue(dist);
                }
            }
        }
    }

    if (mode_2d) {
        if (!ui->checkBox_clip_2d_mode->isChecked()) {
            ui->checkBox_clip_2d_mode->setChecked(true);
        }
    }
    else {
        if (ui->checkBox_clip_2d_mode->isChecked()) {
            ui->checkBox_clip_2d_mode->setChecked(false);
        }
    }
    suppress_clipping.reset();
    updateClipping();
}

void ClippingCtrl::applyClippingDirect(const std::vector<Planef>& planes, bool any_min_priority, bool mode_2d)
{
    Clipping clipping(ui->obj3dViewer->sceneView()->sceneGraph());

    std::vector<Clipping::PlaneInfo> plane_info;

    std::vector<Clipping::TargetNode> target_list;
    clipping.collectTarget(ui->obj3dViewer->sceneView()->sceneGraph()->rootNode(), Matrix4x4f(), target_list, true);

    for (auto& plane : planes) {
        plane_info.emplace_back(plane);
        plane_info.back().m_only_get_plane = mode_2d;
        plane_info.back().setPlaneInfo();
    }
    clipping.execute(plane_info, target_list);
    ui->obj3dViewer->setClippingPlanes(plane_info);
}

bool ClippingCtrl::setRangeAny(bool keep_setting_value)
{
    float         step_size;
    float         cell_size = 1.0f;
    BoundingBox3f bbox      = boxAndStepSize(step_size, cell_size);

    float norm_x = (float)ui->doubleSpinBox_ClipAnyDirX->value();
    float norm_y = (float)ui->doubleSpinBox_ClipAnyDirY->value();
    float norm_z = (float)ui->doubleSpinBox_ClipAnyDirZ->value();

    if (ui->radioButton_ClipAnyDirAngle->isChecked()) {
        /// 角度⇒法線
        double theta = degreeToRadian(ui->doubleSpinBox_ClipAnyDirX->value());
        double phi   = degreeToRadian(ui->doubleSpinBox_ClipAnyDirY->value());

        norm_x = std::sin(theta) * std::cos(phi);
        norm_y = std::sin(theta) * std::sin(phi);
        norm_z = std::cos(theta);
    }

    if (norm_x == 0.0f && norm_y == 0.0f && norm_z == 0.0f) {
        if (ui->radioButton_ClipScale_Real->isChecked()) {
            ui->horizontalSlider_ClipAny->init(0, 0, step_size * cell_size * m_disp_unit, 4, m_lower_upper_diff_min_nm);
        }
        else {
            ui->horizontalSlider_ClipAny->init(0, 0, cell_size, 4, m_lower_upper_diff_min_cell);
        }
        return false;
    }

    float org_x = (float)ui->doubleSpinBox_ClipAnyPosX->value();
    float org_y = (float)ui->doubleSpinBox_ClipAnyPosY->value();
    float org_z = (float)ui->doubleSpinBox_ClipAnyPosZ->value();
    if (!ui->radioButton_ClipScale_Real->isChecked()) {
        org_x *= m_step;
        org_y *= m_step;
        org_z *= m_step;
    }
    else {
        org_x /= m_disp_unit;
        org_y /= m_disp_unit;
        org_z /= m_disp_unit;
    }

    Planef plane;
    plane.setFromNormalAndPoint(Point3f(norm_x, norm_y, norm_z), Point3f(org_x, org_y, org_z));

    float min_pos = FLT_MAX;
    float max_pos = -FLT_MAX;
    for (int ic = 0; ic < 8; ++ic) {
        float dist = plane.distanceToPoint(bbox.corner(ic));
        if (dist < min_pos) {
            min_pos = dist;
        }
        if (dist > max_pos) {
            max_pos = dist;
        }
    }

    if (keep_setting_value) {
        /// 　変更なければ
        if (m_planes[PlaneAnyMin].m_plane.normal() == Point3f(norm_x, norm_y, norm_z).normalized()
            && m_any_origin == Point3f(org_x, org_y, org_z)) {
        }
        else {
            if (ui->radioButton_ClipScale_Real->isChecked()) {
                ui->horizontalSlider_ClipAny->init(min_pos * m_disp_unit, max_pos * m_disp_unit,
                                                   step_size * cell_size * m_disp_unit, 4, m_lower_upper_diff_min_nm);
            }
            else {
                ui->horizontalSlider_ClipAny->init(min_pos / step_size, max_pos / step_size, cell_size, 4,
                                                   m_lower_upper_diff_min_cell);
            }
        }
    }
    else {
        if (ui->radioButton_ClipScale_Real->isChecked()) {
            ui->horizontalSlider_ClipAny->init(min_pos * m_disp_unit, max_pos * m_disp_unit,
                                               step_size * cell_size * m_disp_unit, 4, m_lower_upper_diff_min_nm);
        }
        else {
            ui->horizontalSlider_ClipAny->init(min_pos / step_size, max_pos / step_size, cell_size, 4,
                                               m_lower_upper_diff_min_cell);
        }
    }

    /// maxが調整されるので再取得
    if (ui->radioButton_ClipScale_Real->isChecked()) {
        max_pos = ui->horizontalSlider_ClipAny->maxRealValue() / m_disp_unit;
    }
    else {
        max_pos = ui->horizontalSlider_ClipAny->maxRealValue() * step_size;
    }

    auto point1 = plane.normal() * min_pos + Point3f(org_x, org_y, org_z);

    auto point2 = plane.normal() * max_pos + Point3f(org_x, org_y, org_z);

    m_planes[PlaneAnyMin].m_plane.setFromNormalAndPoint(Point3f(norm_x, norm_y, norm_z), point1);
    m_planes[PlaneAnyMax].m_plane.setFromNormalAndPoint(Point3f(-norm_x, -norm_y, -norm_z), point2);
    m_any_origin = Point3f(org_x, org_y, org_z);

    return true;
}

void ClippingCtrl::anyValueChanged()
{
    if (m_suppress_update_clipping) {
        return;
    }

    SuppressUpdateClipping suppress_clipping(this);

    bool lower = ui->pushButton_ClipAnyMin->isChecked();
    bool upper = ui->pushButton_ClipAnyMax->isChecked();

    float low_rate   = ui->horizontalSlider_ClipAny->lowPosRate();
    float upper_rate = ui->horizontalSlider_ClipAny->upperPosRate();

    // double lower_value = ui->horizontalSlider_ClipAny->lowerSpinBox()->value();
    // double upper_value = ui->horizontalSlider_ClipAny->upperSpinBox()->value();
    if (setRangeAny(true)) {
        if (lower || upper) {
            if (lower) {
                ui->horizontalSlider_ClipAny->setEnableLower(lower);
                // ui->horizontalSlider_ClipAny->lowerSpinBox()->setValue(lower_value);
                ui->horizontalSlider_ClipAny->setLowPosRate(low_rate);
            }
            if (upper) {
                ui->horizontalSlider_ClipAny->setEnableUpper(upper);
                // ui->horizontalSlider_ClipAny->upperSpinBox()->setValue(upper_value);
                ui->horizontalSlider_ClipAny->setUpperPosRate(upper_rate);
            }
            suppress_clipping.reset();
            updateClipping();
        }
    }
    suppress_clipping.reset();
}

void ClippingCtrl::updateClipping(bool force_update)
{
    if (m_suppress_update_clipping) {
        return;
    }

    /// 処理共通化
    std::vector<std::tuple<PlaneIndex, QDoubleSpinBox*, int>> clipping_controls = {
        {PlaneZMin,   ui->doubleSpinBox_ClipZMin,   2}, /// Z平面で最初に切る
        {PlaneZMax,   ui->doubleSpinBox_ClipZMax,   2},
        {PlaneXMin,   ui->doubleSpinBox_ClipXMin,   0},
        {PlaneXMax,   ui->doubleSpinBox_ClipXMax,   0},
        {PlaneYMin,   ui->doubleSpinBox_ClipYMin,   1},
        {PlaneYMax,   ui->doubleSpinBox_ClipYMax,   1},
        {PlaneAnyMin, ui->doubleSpinBox_ClipAnyMin, 3},
        {PlaneAnyMax, ui->doubleSpinBox_ClipAnyMax, 3},
    };

    /// Any 原点
    float any_org_x = (float)ui->doubleSpinBox_ClipAnyPosX->value();
    float any_org_y = (float)ui->doubleSpinBox_ClipAnyPosY->value();
    float any_org_z = (float)ui->doubleSpinBox_ClipAnyPosZ->value();
    if (!ui->radioButton_ClipScale_Real->isChecked()) {
        any_org_x *= m_step;
        any_org_y *= m_step;
        any_org_z *= m_step;
    }
    else {
        any_org_x /= m_disp_unit;
        any_org_y /= m_disp_unit;
        any_org_z /= m_disp_unit;
    }

    bool                             change = false;
    std::vector<Clipping::PlaneInfo> planes;
    for (int ic = 0; ic < clipping_controls.size(); ++ic) {
        PlaneIndex      plane_index = std::get<0>(clipping_controls[ic]);
        QDoubleSpinBox* spin_box    = std::get<1>(clipping_controls[ic]);
        int             axis_index  = std::get<2>(clipping_controls[ic]);

        if (m_planes[plane_index].m_enable != spin_box->isEnabled()) {
            m_planes[plane_index].m_enable = spin_box->isEnabled();
        }
        if (m_planes[plane_index].m_enable) {
            float value = (float)spin_box->value();
            if (!ui->radioButton_ClipScale_Real->isChecked()) {
                value = m_step * value;
            }
            else {
                value /= m_disp_unit;
            }
            Point3f point(axis_index == 0 ? value : 0, axis_index == 1 ? value : 0, axis_index == 2 ? value : 0);

            /// 任意面の場合
            if (axis_index == 3) {
                point = m_planes[PlaneAnyMin].m_plane.normal() * value + Point3f(any_org_x, any_org_y, any_org_z);
            }

            if (0.0 != m_planes[plane_index].m_plane.distanceToPoint(point)) {
                m_planes[plane_index].m_plane.setFromPoint(point);
            }
            planes.emplace_back(m_planes[plane_index].m_plane);

            if (ui->checkBox_clip_2d_mode->isChecked()) {
                planes.back().m_only_get_plane = true;
            }
            else {
                switch (plane_index) {
                    case PlaneXMin:
                    case PlaneXMax: {
                        if (ui->checkBox_ClipX2d->isChecked()) {
                            planes.back().m_only_get_plane = true;
                        }
                    } break;
                    case PlaneYMin:
                    case PlaneYMax: {
                        if (ui->checkBox_ClipY2d->isChecked()) {
                            planes.back().m_only_get_plane = true;
                        }
                    } break;
                    case PlaneZMin:
                    case PlaneZMax: {
                        if (ui->checkBox_ClipZ2d->isChecked()) {
                            planes.back().m_only_get_plane = true;
                        }
                    } break;
                    case PlaneAnyMin:
                    case PlaneAnyMax: {
                        if (ui->checkBox_ClipAny2d->isChecked()) {
                            planes.back().m_only_get_plane = true;
                        }
                    } break;
                }
            }
        }
    }

    if (m_current_planes.size() != planes.size()) {
        change = true;
    }
    else {
        for (int ic = 0; ic < m_current_planes.size(); ++ic) {
            if (planes[ic].m_plane.normal() != m_current_planes[ic].normal()) {
                change = true;
                break;
            }
            if (planes[ic].m_plane.origin() != m_current_planes[ic].origin()) {
                change = true;
                break;
            }
        }
    }

    Clipping clipping(ui->obj3dViewer->sceneView()->sceneGraph());

    std::vector<Clipping::TargetNode> target_list;

    if (!planes.empty()) {
        /// 対象の変更を検知
        clipping.collectTarget(nullptr, Matrix4x4f(), target_list, true);

        /// 対象が以前の対象に含まれているか
        std::set<WeakRefPtr<Node>> target_nodes;
        for (auto& target : target_list) {
            if (!m_target_nodes.count(target.m_node)) {
                change = true;
            }
            target_nodes.insert(target.m_node);
        }

        /// 必要であれば
        if (!change && !force_update) {
            /// 逆のチェック（以前の対象に含まれていないのがあるか）
            for (auto& target : m_target_nodes) {
                if (!target_nodes.count(target)) {
                    change = true;
                }
            }
        }
        m_target_nodes = target_nodes;
    }
    else {
        if (!m_target_nodes.empty()) {
            change = true;
            m_target_nodes.clear();
        }
    }

    if (change || force_update) {
        if (planes.empty() || m_target_nodes.empty()) {
            DEBUG() << "Clear Clipping";

            Clipping::deleteWorkMemory();

            if (clipping.clear(nullptr, false)) {
                ui->obj3dViewer->updateRenderData();

                /// 断面と自動寸法の連動
                auto dimension_ctrl = m_3DForm->dimensionCtrl();
                if (dimension_ctrl) {
                    dimension_ctrl->updateAutoDimensionOnSection();
                }

                ui->obj3dViewer->update();
            }

            ui->obj3dViewer->clearClippingPlanes();

            /// Resultの方のボタンの有効無効設定
            ui->pushButton_result3d_clear_clipping->setEnabled(false);
        }
        else {
            DEBUG() << "Execute Clipping";

            for (auto& plane_info : planes) {
                plane_info.setPlaneInfo();
            }
            clipping.execute(planes, target_list);

            ui->obj3dViewer->setClippingPlanes(planes);

            ui->obj3dViewer->updateRenderData();

            /// 断面と自動寸法の連動
            auto dimension_ctrl = m_3DForm->dimensionCtrl();
            if (dimension_ctrl) {
                dimension_ctrl->updateAutoDimensionOnSection();
            }

            ui->obj3dViewer->update();

            /// Resultの方のボタンの有効無効設定
            ui->pushButton_result3d_clear_clipping->setEnabled(true);
        }
    }

    m_current_planes.clear();
    for (auto& plane : planes) {
        m_current_planes.emplace_back(plane.m_plane);
    }
}

void ClippingCtrl::deltaXFromVoxel(const Node* node, float& delta, float& cell_size)
{
    for (const auto child : node->children()) {
        auto object = child->object();
        if (object && object->isVoxel()) {
            /// すべて同じはずだが一応
            if (delta > ((Voxel*)object)->originalDX()) {
                delta = ((Voxel*)object)->originalDX();
            }
            if (delta > ((Voxel*)object)->originalDY()) {
                delta = ((Voxel*)object)->originalDY();
            }
            if (delta > ((Voxel*)object)->originalDZ()) {
                delta = ((Voxel*)object)->originalDZ();
            }
        }
        else {
            deltaXFromVoxel(child.ptr(), delta, cell_size);
        }
    }
}

BoundingBox3f ClippingCtrl::boxAndStepSize(float& step_size, float& cell_size, bool init)
{
    /// 領域サイズ設定
    BoundingBox3f bbox(0, 0, 0, 1, 1, 1);
    step_size = 0.1;

    auto scene_view = ui->obj3dViewer->sceneView();
    if (scene_view) {
        auto root_node = scene_view->sceneGraph()->rootNode();

        if (init) {
            auto root_box = root_node->shapeBoundingBox();
            if (root_box.valid()) {
                bbox = root_box;

                /// 微小誤差考慮
                Point3f min_pos = bbox.min_pos();
                Point3f max_pos = bbox.max_pos();
                if (fabs(min_pos.x()) < 1.0e-6f) {
                    min_pos.setX(0.0f);
                }
                if (fabs(min_pos.y()) < 1.0e-6f) {
                    min_pos.setY(0.0f);
                }
                if (fabs(min_pos.z()) < 1.0e-6f) {
                    min_pos.setZ(0.0f);
                }
                if (max_pos.x() < min_pos.x()) {
                    max_pos.setX(min_pos.x());
                }
                if (max_pos.y() < min_pos.y()) {
                    max_pos.setY(min_pos.y());
                }
                if (max_pos.z() < min_pos.z()) {
                    max_pos.setZ(min_pos.z());
                }
                bbox.set(min_pos, max_pos);
            }
            m_all_box = bbox;

            /// 暫定：縮小したときにピック領域が大きくなりすぎるので高速化：画面サイズとってあわせるかそもそもを直すか（速くするか）したい
            float max_size = std::max(m_all_box.getXLength(), std::max(m_all_box.getYLength(), m_all_box.getZLength()));
            ui->obj3dViewer->setMaxPickLength(max_size / 300);
        }
        else {
            bbox = m_all_box;
        }

        /// 最小のデルタにする
        float delta = FLT_MAX;
        float cell  = FLT_MAX;
        deltaXFromVoxel(root_node, delta, cell);
        if (delta > 0 && delta != FLT_MAX) {
            step_size = delta;
        }
        if (cell > 0 && cell != FLT_MAX) {
            cell_size = cell;
        }
    }

    return bbox;
}

void ClippingCtrl::changeScaleRealCell()
{
    SuppressUpdateClipping suppress_clipping(this);
    ui->ClippingTab->setUpdatesEnabled(false);

    float         step_size;
    float         cell_size = 1.0f;
    BoundingBox3f bbox      = boxAndStepSize(step_size, cell_size);

    bool  enable_lower_x = ui->horizontalSlider_ClipX->isEnableLower();
    float lower_value_x  = ui->horizontalSlider_ClipX->lowerRealValue();
    bool  enable_upper_x = ui->horizontalSlider_ClipX->isEnableUpper();
    float upper_value_x  = ui->horizontalSlider_ClipX->upperRealValue();

    bool  enable_lower_y = ui->horizontalSlider_ClipY->isEnableLower();
    float lower_value_y  = ui->horizontalSlider_ClipY->lowerRealValue();
    bool  enable_upper_y = ui->horizontalSlider_ClipY->isEnableUpper();
    float upper_value_y  = ui->horizontalSlider_ClipY->upperRealValue();

    bool  enable_lower_z = ui->horizontalSlider_ClipZ->isEnableLower();
    float lower_value_z  = ui->horizontalSlider_ClipZ->lowerRealValue();
    bool  enable_upper_z = ui->horizontalSlider_ClipZ->isEnableUpper();
    float upper_value_z  = ui->horizontalSlider_ClipZ->upperRealValue();

    bool  enable_lower_a = ui->horizontalSlider_ClipAny->isEnableLower();
    float lower_value_a  = ui->horizontalSlider_ClipAny->lowerRealValue();
    bool  enable_upper_a = ui->horizontalSlider_ClipAny->isEnableUpper();
    float upper_value_a  = ui->horizontalSlider_ClipAny->upperRealValue();

    if (ui->radioButton_ClipScale_Real->isChecked()) {
        ui->horizontalSlider_ClipX->init(bbox.xMin() * m_disp_unit, bbox.xMax() * m_disp_unit,
                                         step_size * cell_size * m_disp_unit, -1, m_lower_upper_diff_min_nm);
        ui->horizontalSlider_ClipY->init(bbox.yMin() * m_disp_unit, bbox.yMax() * m_disp_unit,
                                         step_size * cell_size * m_disp_unit, -1, m_lower_upper_diff_min_nm);
        ui->horizontalSlider_ClipZ->init(bbox.zMin() * m_disp_unit, bbox.zMax() * m_disp_unit,
                                         step_size * cell_size * m_disp_unit, -1, m_lower_upper_diff_min_nm);

        lower_value_x *= (step_size * m_disp_unit);
        lower_value_y *= (step_size * m_disp_unit);
        lower_value_z *= (step_size * m_disp_unit);
        lower_value_a *= (step_size * m_disp_unit);

        upper_value_x *= (step_size * m_disp_unit);
        upper_value_y *= (step_size * m_disp_unit);
        upper_value_z *= (step_size * m_disp_unit);
        upper_value_a *= (step_size * m_disp_unit);

        /// stepの小数点以下の桁数を計算
        int decimals = 0;
        if (step_size != static_cast<int>(step_size)) {
            QString stepStr  = QString::number(step_size, 'f', 6);
            int     dotIndex = stepStr.indexOf('.');
            if (dotIndex != -1) {
                decimals = stepStr.length() - dotIndex - 1;
                while (decimals > 0 && stepStr.endsWith('0')) {
                    stepStr.chop(1);
                    decimals--;
                }
            }
        }
        /// 暫定
        if (decimals < 2) {
            decimals = 2;
        }
        else if (decimals < 4) {
            decimals = 4;
        }

        ui->doubleSpinBox_ClipAnyPosX->setDecimals(4);
        ui->doubleSpinBox_ClipAnyPosY->setDecimals(4);
        ui->doubleSpinBox_ClipAnyPosZ->setDecimals(4);
        ui->doubleSpinBox_ClipAnyPosX->setSingleStep(step_size * cell_size * m_disp_unit);
        ui->doubleSpinBox_ClipAnyPosY->setSingleStep(step_size * cell_size * m_disp_unit);
        ui->doubleSpinBox_ClipAnyPosZ->setSingleStep(step_size * cell_size * m_disp_unit);
        ui->doubleSpinBox_ClipAnyPosX->setValue(ui->doubleSpinBox_ClipAnyPosX->value() * (step_size * m_disp_unit));
        ui->doubleSpinBox_ClipAnyPosY->setValue(ui->doubleSpinBox_ClipAnyPosY->value() * (step_size * m_disp_unit));
        ui->doubleSpinBox_ClipAnyPosZ->setValue(ui->doubleSpinBox_ClipAnyPosZ->value() * (step_size * m_disp_unit));
    }
    else {
        ui->horizontalSlider_ClipX->init(bbox.xMin() / step_size, bbox.xMax() / step_size, cell_size, -1,
                                         m_lower_upper_diff_min_cell);
        ui->horizontalSlider_ClipY->init(bbox.yMin() / step_size, bbox.yMax() / step_size, cell_size, -1,
                                         m_lower_upper_diff_min_cell);
        ui->horizontalSlider_ClipZ->init(bbox.zMin() / step_size, bbox.zMax() / step_size, cell_size, -1,
                                         m_lower_upper_diff_min_cell);

        lower_value_x /= (step_size * m_disp_unit);
        lower_value_y /= (step_size * m_disp_unit);
        lower_value_z /= (step_size * m_disp_unit);
        lower_value_a /= (step_size * m_disp_unit);

        upper_value_x /= (step_size * m_disp_unit);
        upper_value_y /= (step_size * m_disp_unit);
        upper_value_z /= (step_size * m_disp_unit);
        upper_value_a /= (step_size * m_disp_unit);

        ui->doubleSpinBox_ClipAnyPosX->setValue(ui->doubleSpinBox_ClipAnyPosX->value() / (step_size * m_disp_unit));
        ui->doubleSpinBox_ClipAnyPosY->setValue(ui->doubleSpinBox_ClipAnyPosY->value() / (step_size * m_disp_unit));
        ui->doubleSpinBox_ClipAnyPosZ->setValue(ui->doubleSpinBox_ClipAnyPosZ->value() / (step_size * m_disp_unit));
        ui->doubleSpinBox_ClipAnyPosX->setDecimals(4);    /// 暫定 4桁固定でいい
        ui->doubleSpinBox_ClipAnyPosY->setDecimals(4);
        ui->doubleSpinBox_ClipAnyPosZ->setDecimals(4);
        ui->doubleSpinBox_ClipAnyPosX->setSingleStep(cell_size);
        ui->doubleSpinBox_ClipAnyPosY->setSingleStep(cell_size);
        ui->doubleSpinBox_ClipAnyPosZ->setSingleStep(cell_size);
    }

    m_step = step_size;

    setRangeAny(false);

    if (enable_lower_x) {
        ui->horizontalSlider_ClipX->setEnableLower(enable_lower_x);
        ui->horizontalSlider_ClipX->setLowerRealValue(lower_value_x);
    }
    if (enable_upper_x) {
        ui->horizontalSlider_ClipX->setEnableUpper(enable_upper_x);
        ui->horizontalSlider_ClipX->setUpperRealValue(upper_value_x);
    }

    if (enable_lower_y) {
        ui->horizontalSlider_ClipY->setEnableLower(enable_lower_y);
        ui->horizontalSlider_ClipY->setLowerRealValue(lower_value_y);
    }
    if (enable_upper_y) {
        ui->horizontalSlider_ClipY->setEnableUpper(enable_upper_y);
        ui->horizontalSlider_ClipY->setUpperRealValue(upper_value_y);
    }

    if (enable_lower_z) {
        ui->horizontalSlider_ClipZ->setEnableLower(enable_lower_z);
        ui->horizontalSlider_ClipZ->setLowerRealValue(lower_value_z);
    }
    if (enable_upper_z) {
        ui->horizontalSlider_ClipZ->setEnableUpper(enable_upper_z);
        ui->horizontalSlider_ClipZ->setUpperRealValue(upper_value_z);
    }

    if (enable_lower_a) {
        ui->horizontalSlider_ClipAny->setEnableLower(enable_lower_a);
        ui->horizontalSlider_ClipAny->setLowerRealValue(lower_value_a);
    }
    if (enable_upper_a) {
        ui->horizontalSlider_ClipAny->setEnableUpper(enable_upper_a);
        ui->horizontalSlider_ClipAny->setUpperRealValue(upper_value_a);
    }

    ui->ClippingTab->setUpdatesEnabled(true);
    suppress_clipping.reset();
    anyValueChanged();
    updateClipping();
}

void ClippingCtrl::changeAnyDirAngle()
{
    SuppressUpdateClipping suppress_clipping(this);
    ui->ClippingTab->setUpdatesEnabled(false);
    if (ui->radioButton_ClipAnyDirNorm->isChecked()) {
        ui->label_ClipAny_DirX->setText("X");
        ui->label_ClipAny_DirY->setText("Y");
        ui->label_ClipAny_DirZ->setVisible(true);
        ui->label_ClipAnyDirAngle->setVisible(false);
        ui->doubleSpinBox_ClipAnyDirZ->setVisible(true);

        /// 角度⇒法線
        double theta = degreeToRadian(ui->doubleSpinBox_ClipAnyDirX->value());
        double phi   = degreeToRadian(ui->doubleSpinBox_ClipAnyDirY->value());

        double x = std::sin(theta) * std::cos(phi);
        double y = std::sin(theta) * std::sin(phi);
        double z = std::cos(theta);

        /// 一番大きい値を1に調整
        double max_xyz = std::max(x, std::max(y, z));
        if (max_xyz > 0) {
            x /= max_xyz;
            y /= max_xyz;
            z /= max_xyz;
        }

        ui->doubleSpinBox_ClipAnyDirX->setDecimals(4);
        ui->doubleSpinBox_ClipAnyDirY->setDecimals(4);
        ui->doubleSpinBox_ClipAnyDirZ->setDecimals(4);
        ui->doubleSpinBox_ClipAnyDirX->setSingleStep(0.1);
        ui->doubleSpinBox_ClipAnyDirY->setSingleStep(0.1);
        ui->doubleSpinBox_ClipAnyDirZ->setSingleStep(0.1);
        ui->doubleSpinBox_ClipAnyDirX->setValue(x);
        ui->doubleSpinBox_ClipAnyDirY->setValue(y);
        ui->doubleSpinBox_ClipAnyDirZ->setValue(z);
    }
    else {
        ui->label_ClipAny_DirX->setText("Θ");
        ui->label_ClipAny_DirY->setText("φ");
        ui->label_ClipAny_DirZ->setVisible(false);
        ui->label_ClipAnyDirAngle->setVisible(true);
        ui->doubleSpinBox_ClipAnyDirZ->setVisible(false);

        /// 法線⇒角度
        double norm_x = ui->doubleSpinBox_ClipAnyDirX->value();
        double norm_y = ui->doubleSpinBox_ClipAnyDirY->value();
        double norm_z = ui->doubleSpinBox_ClipAnyDirZ->value();

        double theta, phi;
        auto   r = std::sqrt(norm_x * norm_x + norm_y * norm_y + norm_z * norm_z);
        if (r != 0) {
            theta = std::acos(norm_z / r);
        }
        else {
            theta = 0.0;
        }

        if (norm_x == 0.0 && norm_y == 0.0) {
            phi = 0.0;
        }
        else {
            phi = std::atan2(norm_y, norm_x);
        }

        ui->doubleSpinBox_ClipAnyDirX->setDecimals(2);
        ui->doubleSpinBox_ClipAnyDirY->setDecimals(2);
        ui->doubleSpinBox_ClipAnyDirX->setSingleStep(1);
        ui->doubleSpinBox_ClipAnyDirY->setSingleStep(1);
        ui->doubleSpinBox_ClipAnyDirX->setValue(radianToDegree(theta));
        ui->doubleSpinBox_ClipAnyDirY->setValue(radianToDegree(phi));
    }
    ui->ClippingTab->setUpdatesEnabled(true);
    suppress_clipping.reset();
    anyValueChanged();
    updateClipping();
}

void ClippingCtrl::initAnyPreset()
{
    m_any_presets.clear();

    m_any_presets.append(qMakePair(QString(""), AnyDir(Point3f(0, 0, 0), false)));
    m_any_presets.append(qMakePair(QString("＋X ＋Y"), AnyDir(Point3f(1, 1, 0), false)));
    m_any_presets.append(qMakePair(QString("＋X －Y"), AnyDir(Point3f(1, -1, 0), false)));
    m_any_presets.append(qMakePair(QString("＋Y ＋Z"), AnyDir(Point3f(0, 1, 1), false)));
    m_any_presets.append(qMakePair(QString("＋Y －Z"), AnyDir(Point3f(0, 1, -1), false)));
    m_any_presets.append(qMakePair(QString("＋Z ＋X"), AnyDir(Point3f(1, 0, 1), false)));
    m_any_presets.append(qMakePair(QString("＋Z －X"), AnyDir(Point3f(-1, 0, 1), false)));
    m_any_presets.append(qMakePair(QString("＋X ＋Y ＋Z"), AnyDir(Point3f(1, 1, 1), false)));
    m_any_presets.append(qMakePair(QString("＋X －Y ＋Z"), AnyDir(Point3f(1, -1, 1), false)));
    m_any_presets.append(qMakePair(QString("＋X ＋Y －Z"), AnyDir(Point3f(1, 1, -1), false)));
    m_any_presets.append(qMakePair(QString("＋X －Y －Z"), AnyDir(Point3f(1, -1, -1), false)));

    QStringList combobox_texts;
    for (auto& preset : m_any_presets) {
        combobox_texts.append(preset.first);
    }

    ui->comboBox_any_clip_preset->addItems(combobox_texts);

    /// 設定タブ
    ui->comboBox_any_clip_preset_default->addItems(combobox_texts);

    /// サイズ調整
    QFontMetrics fontMetrics(ui->comboBox_any_clip_preset->font());
    int          maxWidth = 0;
    for (const auto& text : combobox_texts) {
        maxWidth = std::max(maxWidth, fontMetrics.horizontalAdvance(text));
    }
    ui->comboBox_any_clip_preset->view()->setMinimumWidth(maxWidth + 40);
}

void ClippingCtrl::setAnyPreset()
{
    int index = ui->comboBox_any_clip_preset->currentIndex();

    if (index < 0 || index >= m_any_presets.size()) {
        return;
    }

    const auto& any_dir = m_any_presets[index].second;
    const auto& norm    = any_dir.norm();
    if (norm.length2() == 0.0) {
        return;
    }

    SuppressUpdateClipping suppress_clipping(this);

    if (ui->radioButton_ClipAnyDirNorm->isChecked()) {
        ui->doubleSpinBox_ClipAnyDirX->setValue(norm.x());
        ui->doubleSpinBox_ClipAnyDirY->setValue(norm.y());
        ui->doubleSpinBox_ClipAnyDirZ->setValue(norm.z());
    }
    else {
        const auto& angle = any_dir.angle();
        ui->doubleSpinBox_ClipAnyDirX->setValue(angle.x());
        ui->doubleSpinBox_ClipAnyDirY->setValue(angle.y());
    }

    suppress_clipping.reset();
    anyValueChanged();
    updateClipping();
}

std::vector<std::pair<ClippingCtrl::PlaneIndex, Planef>> ClippingCtrl::currentValidPlanes(const BoundingBox3f& bbox,
                                                                                          bool get_invalid_any)
{
    /*
    BoundingBox3f bbox;

    auto scene_view = ui->obj3dViewer->sceneView();
    if (scene_view) {
        bbox = scene_view->sceneGraph()->rootNode()->shapeDisplayBoundingBox();
    }
    */

    /// 表示なし
    if (!bbox.valid()) {
        return {};
    }

    std::vector<std::pair<PlaneIndex, Planef>> planes;

    for (int index = 0; index < PlaneIndex::NumOfPlane; ++index) {
        Planef plane;
        switch (index) {
            case PlaneIndex::PlaneXMin:
            case PlaneIndex::PlaneYMin:
            case PlaneIndex::PlaneZMin:
            case PlaneIndex::PlaneAnyMin: {
                const auto& clip_plane = m_planes[index].m_plane;
                if (m_planes[index].m_enable) {
                    // if (clip_plane.distanceToPoint(bbox.min_pos()) > 0) {
                    //     plane.setFromNormalAndPoint(clip_plane.normal(), bbox.min_pos());
                    // }
                    // else {
                    plane = clip_plane;
                    //}
                }
                else if (index != PlaneIndex::PlaneAnyMin) {
                    plane.setFromNormalAndPoint(clip_plane.normal(), bbox.min_pos());
                }
                else if (get_invalid_any && index == PlaneIndex::PlaneAnyMin) {
                    if (clip_plane.normal().length2() > 0) {
                        int   min_corner = 0;
                        float min_pos    = FLT_MAX;
                        for (int ic = 0; ic < 8; ++ic) {
                            float dist = clip_plane.distanceToPoint(bbox.corner(ic));
                            if (dist < min_pos) {
                                min_pos    = dist;
                                min_corner = ic;
                            }
                        }
                        plane.setFromNormalAndPoint(clip_plane.normal(), bbox.corner(min_corner));
                    }
                }
                else {
                    continue;
                }
            } break;
            case PlaneIndex::PlaneXMax:
            case PlaneIndex::PlaneYMax:
            case PlaneIndex::PlaneZMax:
            case PlaneIndex::PlaneAnyMax: {
                const auto& clip_plane = m_planes[index].m_plane;
                if (m_planes[index].m_enable) {
                    // if (clip_plane.distanceToPoint(bbox.max_pos()) > 0) {
                    //     plane.setFromNormalAndPoint(clip_plane.normal(), bbox.max_pos());
                    // }
                    // else {
                    plane = clip_plane;
                    //}
                }
                else if (index != PlaneIndex::PlaneAnyMax) {
                    plane.setFromNormalAndPoint(clip_plane.normal(), bbox.max_pos());
                }
                else if (get_invalid_any && index == PlaneIndex::PlaneAnyMax) {
                    if (clip_plane.normal().length2() > 0) {
                        int   min_corner = 0;    /// 法線逆なので同じ
                        float min_pos    = FLT_MAX;
                        for (int ic = 0; ic < 8; ++ic) {
                            float dist = clip_plane.distanceToPoint(bbox.corner(ic));
                            if (dist < min_pos) {
                                min_pos    = dist;
                                min_corner = ic;
                            }
                        }
                        plane.setFromNormalAndPoint(clip_plane.normal(), bbox.corner(min_corner));
                    }
                }
                else {
                    continue;
                }
            } break;
        }

        planes.emplace_back((PlaneIndex)index, plane);
    }

    return planes;
}

Point3f ClippingCtrl::AnyDir::norm() const
{
    if (!m_angle) {
        return m_dir_or_angle;
    }
    else {
        /// 角度⇒法線
        double theta = degreeToRadian(m_dir_or_angle.x());
        double phi   = degreeToRadian(m_dir_or_angle.y());

        double x = std::sin(theta) * std::cos(phi);
        double y = std::sin(theta) * std::sin(phi);
        double z = std::cos(theta);

        /// 一番大きい値を1に調整
        double max_xyz = std::max(x, std::max(y, z));
        if (max_xyz > 0) {
            x /= max_xyz;
            y /= max_xyz;
            z /= max_xyz;
        }

        return Point3f(x, y, z);
    }
}

Point3f ClippingCtrl::AnyDir::angle() const
{
    if (m_angle) {
        return m_dir_or_angle;
    }
    else {
        /// 法線⇒角度
        double norm_x = m_dir_or_angle.x();
        double norm_y = m_dir_or_angle.y();
        double norm_z = m_dir_or_angle.z();

        double theta, phi;
        auto   r = std::sqrt(norm_x * norm_x + norm_y * norm_y + norm_z * norm_z);
        if (r != 0) {
            theta = std::acos(norm_z / r);
        }
        else {
            theta = 0.0;
        }

        if (norm_x == 0.0 && norm_y == 0.0) {
            phi = 0.0;
        }
        else {
            phi = std::atan2(norm_y, norm_x);
        }

        radianToDegree(theta);
        radianToDegree(phi);

        return Point3f(radianToDegree(theta), radianToDegree(phi), 0);
    }
}
