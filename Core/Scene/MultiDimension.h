#ifndef CORE_MULTIDIMENSION_H
#define CORE_MULTIDIMENSION_H

#include <set>
#include "Annotation.h"
#include "CoreGlobal.h"
#include "Dimension.h"

CORE_NAMESPACE_BEGIN

/// TODO: 暫定

class CORE_EXPORT MultiDimension : public Annotation {
    DECLARE_META_OBJECT(MultiDimension)

public:
    struct DimensionInfo {
        RefPtr<Dimension> m_dimension;
        std::wstring      m_material;
        std::wstring      m_material_original;

        DimensionInfo(RefPtr<Dimension>& dimension, const std::wstring& material, const std::wstring& material_original)
            : m_dimension(dimension)
            , m_material(material)
            , m_material_original(material_original)
        {
        }
    };

    void addDimensions(const std::vector<DimensionInfo>& dimensions);
    void popbackDimensions(int count);
    void popbackBasePoints(int count);

    const std::vector<DimensionInfo>& dimensions() const { return m_dimensions; }
    std::vector<DimensionInfo>&       dimensions() { return m_dimensions; }

    virtual ObjectType    type() const override { return ObjectType::MultiDimension; }
    virtual void          updateBoundingBox() override;
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const override;

    virtual void setColor(float r, float g, float b) override;
    virtual void setColor(const Point4f& color) override;

    void  setLineWidth(float line_width);
    float lineWidth() const;

    void setDisplayUnit(bool display);
    bool isDisplayUnit();

    void                 setTextAlign(Dimension::TextAlign text_align);
    Dimension::TextAlign textAlign() const;

    std::vector<std::wstring> textList() const;

    const std::wstring& material(int index) const;
    const std::wstring& materialOriginal(int index) const;
    void                setMaterial(int index, const std::wstring& material);
    void                setMaterial(const std::map<std::wstring, std::wstring>& material_name_change);
    std::wstring        valueString(int index) const;
    int                 count() const { return m_dimensions.size(); }

    void setStrSize(int size);
    int  strSize();

    void initStrPosStart();

    void reverse();

    void setDisplayValue(bool display);
    void setDisplayMaterial(bool display);
    void setOnSectionPlane(bool on_plane);
    bool isDisplayValue() const { return m_display_value; }
    bool isDisplayMaterial() const { return m_display_material; }
    bool isOnSectionPlane() const { return m_on_plane; }

    Point3f posStart() const;
    Point3f posEnd() const;

    void updateText();
    void uptateText(int index);
    void updateEdgeDisp(float eps = 1.0e-5f);

    void              setTemporary(bool temporary) { m_temporary = temporary; }
    bool              isTemporary() const { return m_temporary; }
    void              setExtendStart(bool extend) { m_extend_start = extend; }
    bool              isExtendStart() const { return m_extend_start; }
    void              setExtendEnd(bool extend) { m_extend_end = extend; }
    bool              isExtendEnd() const { return m_extend_end; }
    const VecPoint3f& basePoints() const { return m_base_points; }

    enum class TemporaryMode { None = 0, X, Y, Z, Vertical, Horizontal };
    void          setTemporaryModeX() { m_temporary_mode = TemporaryMode::X; }
    void          setTemporaryModeY() { m_temporary_mode = TemporaryMode::Y; }
    void          setTemporaryModeZ() { m_temporary_mode = TemporaryMode::Z; }
    void          setTemporaryModeV() { m_temporary_mode = TemporaryMode::Vertical; }
    void          setTemporaryModeH() { m_temporary_mode = TemporaryMode::Horizontal; }
    TemporaryMode temporaryMode() const { return m_temporary_mode; }

    void createBasePoints(float eps);
    bool removeLastLine(Point3f& lastline_start, Point3f& lastline_end);

    struct OnPlaneInfo {
        int    m_clip_index;
        Planef m_plane;
        OnPlaneInfo(int index, const Planef& plane) : m_clip_index(index), m_plane(plane) {}
    };
    void                              createBasePointPlanes(std::vector<OnPlaneInfo>& current_planes, float eps);
    const std::vector<std::set<int>>& basePointPlanes() const { return m_base_point_planes; }

    void setBasePoints(const VecPoint3f& points) { m_base_points = points; }
    void setBasePointPlanes(const std::vector<std::set<int>>& planes) { m_base_point_planes = planes; }

    void                setEdgeType(Dimension::EdgeType edge_type);
    Dimension::EdgeType edgeType() const;

    void setDispName(bool disp);
    bool dispName() const { return m_disp_name_flag; }

    void setName(const std::wstring& name);

    std::wstring name() const;

    Dimension* nameDimension() { return m_name_dimension.ptr(); }

protected:
    MultiDimension();
    MultiDimension(const MultiDimension& other);
    ~MultiDimension();

protected:
    std::vector<DimensionInfo> m_dimensions;
    VecPoint3f                 m_base_points;
    std::vector<std::set<int>>
        m_base_point_planes;    /// base_pointが乗る平面index(GUIと連動してよくない。GUIで持たせるべきか）
    float         m_line_width       = 2.0f;
    bool          m_temporary        = false;
    bool          m_extend_start     = true;
    bool          m_extend_end       = true;
    bool          m_display_value    = true;
    bool          m_display_material = true;
    bool          m_on_plane         = false;
    bool          m_disp_name_flag   = false;
    TemporaryMode m_temporary_mode   = TemporaryMode::None;

    RefPtr<Dimension> m_name_dimension;    /// 暫定:名前表示するだけの用途（Labelとか別クラスにすべき）
};

CORE_NAMESPACE_END

#endif    // CORE_MULTIDIMENSION_H
