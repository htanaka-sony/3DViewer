#ifndef DIMENSIONCTRL_H
#define DIMENSIONCTRL_H

#include <QColor>
#include <QComboBox>
#include <QObject>
#include "ClippingCtrl.h"
#include "Scene/Dimension.h"
#include "Scene/MultiDimension.h"
#include "Scene/Node.h"
#include "Utils/Clipping.h"

namespace Ui {
class Vox3DForm;
}

class Vox3DForm;

using namespace Core;

namespace Core {
class SceneView;
}

class DimensionCtrl : public QObject {
    Q_OBJECT
public:
    enum class CreateDimensionType { Min = 0, X, Y, Z, XYZ, Vertical, Horizontal, Diagonal, VHD };

    DimensionCtrl(Vox3DForm* parent, Ui::Vox3DForm* ui_);

    void clearAutoDimension();
    void clearDimensions();

    void selectDimension(Node* dimension_node);

    bool isAutoDimension1Pick() const;

    bool isCreateDimension2Points() const;
    bool isCreateAutoDimension() const;
    bool isCreateAutoDimension2Points() const;

    void clearTemporaryDimension();
    void clearTemporaryAutoDimension();
    void clearTemporaryPreviewAutoDimension();

    bool createDimension(SceneView* scene_view, const Point3f& point0, const Point3f& point1, bool temporary,
                         bool use_temporary = false);

    bool createAutoDimension(SceneView* scene_view, const std::vector<Point3f>& points, bool temporary,
                             bool continuous_end = false, bool force_along_plane = false, bool use_temporary = false);

    void setCtrlPress(bool press) { m_ctrl_press = press; }
    void setCtrlRelease(bool release) { m_ctrl_release = release; }

    Point4f dimensionColor() const
    {
        return Point4f(m_dimension_color.redF(), m_dimension_color.greenF(), m_dimension_color.blueF(), 1.0f);
    }
    Point4f autoDimensionColor() const
    {
        return Point4f(m_auto_dimension_color.redF(), m_auto_dimension_color.greenF(), m_auto_dimension_color.blueF(),
                       1.0f);
    }

    void setDimensionColor(QColor color) { m_dimension_color = color; }
    void setAutoDimensionColor(QColor color) { m_auto_dimension_color = color; }

    bool    isContinuousAutoDimension() const;
    bool    isDuringContinuousAutoDimension() const;
    Point3f continousAutoDimensionPoint() const;
    void    fixContinuousAutoDimension();

    void reset(bool delete_dimension = false);

    void modeClear();

    void createAutoDimensionGUIUpdate();

    bool updateAutoDimensionOnSection();

    void updateAutoDimensionMaterialNameChange();
    void clearAutoDimensionMaterialNameChange(bool show_msg = false);

    void autoDimensionTextNameChange();
    void autoDimensionTextAllSelect();
    void autoDimensionTextCopy();
    void autoDimensionTextAllCopy();
    void autoDimensionTextAllCopyExceptOrg();

    void showDimension(bool show);
    void showAutoDimension(bool show);

protected:
    CreateDimensionType createDimensionType(const QString& dimension_type) const;
    CreateDimensionType createDimensionType() const;
    CreateDimensionType createAutoDimensionType() const;

    bool isXYZDimension(CreateDimensionType type) const;

    int dimensionStrSize() const;
    int autoDimensionStrSize() const;

    RenderSnap      dimensionSnap() const;
    Dimension::Type dimensionType() const;

    Dimension::TextAlign autoDimensionTextAlign(QComboBox* combobox = nullptr) const;

    void createDimensionList();
    void createAutoDimensionList();
    void createAutoDimensionText();
    void clearAutoDimensionText();

    void addAutoDimension(Node* dimension, int insert_row = -1, bool set_new_name = true);
    void updateAutoDimensions();
    void updateAutoDimension(Node* target);

    void addDimension(Node* dimension);

    void dimensionShow(bool show, bool all);
    void dimensionColorChange();
    void dimensionSizeChange();
    void dimensionLineWidthChange();
    void dimensionEdgeChange();
    void dimensionNameChange();
    void dimensionNameShow(bool show, bool all = false);
    void dimensionLineInitTextPos();
    void removeDimension(bool all = false);

    void autoDimensionShow(bool show, bool all);
    void autoDimensionValueShow(bool show);
    void autoDimensionMaterialShow(bool show);
    void autoDimensionNameChange();
    void autoDimensionNameShow(bool show, bool all = false);
    void autoDimensionColorChange();
    void autoDimensionSizeChange();
    void autoDimensionLineWidthChange();
    void autoDimensionEdgeChange();
    void autoDimensionTextAlignChange();
    void autoDimensionLineInitTextPos();
    void autoDimensionReverse();
    void autoDimensionOnPlane(bool on);
    void removeAutoDimension(bool all = false);
    int  removeAutoDimensionList(Node* node);

    void updateAutoDimensionTextList();
    void addAutoDimensionText(MultiDimension* dimension, int index);

    QColor adjustColor(const QColor& color, int adjustment);

    bool specifyDimensionPoints(SceneView* scene_view, bool auto_dimension, Point3f& point0, Point3f& point1,
                                Dimension::Type& dimension_type, CreateDimensionType& create_type, bool temporary,
                                Planef* check_on_olane = nullptr);

    bool addAutoDimensionInfo(SceneView* scene_view, const Point3f& new_point0, const Point3f& new_point1,
                              const std::vector<Clipping::SectionInfo>&                       sections,
                              const std::vector<std::pair<ClippingCtrl::PlaneIndex, Planef>>* current_valid_planes,
                              const Point3f* check_vec, bool extend_start, bool extend_end,
                              std::vector<MultiDimension::DimensionInfo>& sorted_dimensions_info);

    std::vector<Node*> autoDimensions();

    bool updateAutoDimensionOnSection(SceneView* scene_view, const BoundingBox3f& bbox, const Point3f& new_point0,
                                      const Point3f&                              new_point1,
                                      std::vector<MultiDimension::DimensionInfo>& sorted_dimensions_info);
    void updateAutoDimensionOnSection(SceneView* scene_view, const BoundingBox3f& bbox, Node* target,
                                      const VecPoint3f& base_point_on_planes, bool update);

    bool createAutoDimensionByPoint(bool temprorary);

    bool temporaryDimensionLastPoint(Point3f& pos);
    bool temporaryAutoDimensionLastPoint(Point3f& pos);
    bool temporaryDimensionPoint(Point3f& point0, Point3f& point1);
    bool temporaryAutoDimensionPoint(Point3f& point0, Point3f& point1);

protected:
    Ui::Vox3DForm* ui;

    Vox3DForm* m_3DForm = nullptr;

    /// Dimension
    QColor m_dimension_color      = QColor(255, 255, 0);
    QColor m_auto_dimension_color = QColor(255, 255, 0);

    RefPtr<Node> m_curent_text_auto_dimension;

    RefPtr<Node> m_continuous_auto_dimension;

    int m_dim_count      = 0;
    int m_auto_dim_count = 0;

    bool m_ctrl_press   = false;
    bool m_ctrl_release = false;

    RefPtr<Node> m_temporary_dimension;
    RefPtr<Node> m_temporary_auto_dimension;
    RefPtr<Node> m_temporary_auto_preview_dimension;

    std::map<std::wstring, std::wstring> m_auto_dimension_material_change;
};

#endif    // DIMENSIONCTRL_H
