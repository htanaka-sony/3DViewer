#ifndef CLIPPINGCTRL_H
#define CLIPPINGCTRL_H

#include <QObject>
#include "Math/Planef.h"
#include "Scene/Node.h"

class Vox3DForm;

namespace Ui {
class Vox3DForm;
}

using namespace Core;

class ClippingCtrl : public QObject {
    Q_OBJECT

    friend class SuppressUpdateClipping;

public:
    ClippingCtrl(Vox3DForm* form, Ui::Vox3DForm* ui_);

    void reset(bool keep_setting_value);

    void applyPlanes(const std::vector<Planef>& planes, bool any_min_priority, bool mode_2d);

    void applyClippingDirect(const std::vector<Planef>& planes, bool any_min_priority, bool mode_2d);

    bool setRangeAny(bool keep_setting_value);
    void anyValueChanged();

    void updateClipping(bool force_update = false);

    void deltaXFromVoxel(const Node* node, float& delta, float& cell_size);

    BoundingBox3f boxAndStepSize(float& step_size, float& cell_size, bool init = false);

    void changeScaleRealCell();
    void changeAnyDirAngle();

    void initAnyPreset();
    void setAnyPreset();

    struct PlaneInfo {
        Planef m_plane;
        bool   m_enable = false;
    };
    enum PlaneIndex {
        PlaneXMin = 0,
        PlaneXMax,
        PlaneYMin,
        PlaneYMax,
        PlaneZMin,
        PlaneZMax,
        PlaneAnyMin,
        PlaneAnyMax,
        NumOfPlane
    };

    const PlaneInfo* clipPlanes() { return m_planes; }

    std::vector<std::pair<PlaneIndex, Planef>> currentValidPlanes(const BoundingBox3f& bbox,
                                                                  bool                 get_invalid_any = false);

protected:
    Vox3DForm* m_3DForm = nullptr;

    Ui::Vox3DForm* ui;

    PlaneInfo m_planes[NumOfPlane];    // XYZ+any

    VecPlanef m_current_planes;

    /// Any
    Point3f m_any_origin;    /// 元の原点

    float m_step;

    float  m_disp_unit                 = 1.0e3f;    /// 表示の単位調整
    double m_lower_upper_diff_min_nm   = 0.1;       /// lower と Upperの最小の差 (nm)
    double m_lower_upper_diff_min_cell = 0.1;       /// lower と Upperの最小の差 (セル単位)

    std::set<WeakRefPtr<Node>> m_target_nodes;

    bool m_suppress_update_clipping;

    class SuppressUpdateClipping {
    public:
        SuppressUpdateClipping(ClippingCtrl* ctrl) : m_ctrl(ctrl)
        {
            m_save_suppress_update_clipping    = m_ctrl->m_suppress_update_clipping;
            m_ctrl->m_suppress_update_clipping = true;
        }
        ~SuppressUpdateClipping() { reset(); }

        void reset()
        {
            if (!m_reset) {
                m_ctrl->m_suppress_update_clipping = m_save_suppress_update_clipping;
            }
            m_reset = true;
        }

    protected:
        ClippingCtrl* m_ctrl;
        bool          m_save_suppress_update_clipping;
        bool          m_reset = false;
    };

    struct AnyDir {
        Point3f m_dir_or_angle;
        bool    m_angle;
        AnyDir(const Point3f& point, bool angle) : m_dir_or_angle(point), m_angle(angle) {}

        Point3f norm() const;
        Point3f angle() const;
    };
    QList<QPair<QString, AnyDir>> m_any_presets;

    BoundingBox3f m_all_box;
};

#endif    // CLIPPINGCTRL_H
