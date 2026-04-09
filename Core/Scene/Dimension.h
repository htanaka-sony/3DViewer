#ifndef CORE_DIMENSION_H
#define CORE_DIMENSION_H

#include "Math/Point3d.h"
#include "Math/Point3i.h"

#include "Math/UnitSystem.h"
#include "Scene/Annotation.h"

CORE_NAMESPACE_BEGIN

class TextQuad {
public:
    Point3f m_pos[4];
    bool    isValid() const { return m_pos[0] != m_pos[3]; }
};

class CORE_EXPORT Dimension : public Annotation {
    DECLARE_META_OBJECT(Dimension)
public:
    enum Type { XYZDistance = 0, XDistance, YDistance, ZDistance };

    enum TextAlign { Horizontal = 0, Vertical, Auto };

    enum EdgeType { Circle = 0, LineAuto, LineVertical, LineHorizontal };

    void set(const Point3f& start, const Point3f& end, LengthUnit::Unit base_unit, LengthUnit::Unit disp_unit,
             int decimal, int str_size, Type type = XYZDistance);

    void setTextPosition(const Point3f& pos);

    void setStrSize(int size) { m_str_size = size; }

    void         setTextHeader(const std::wstring& str_header) { m_str_header = str_header; }
    void         updateText();
    std::wstring createText(bool disp_unit = true, bool disp_name = false, bool disp_name_line_break = true,
                            bool disp_value = true) const;

    bool isDisplayUnit() const { return m_disp_unit_flag; }
    void setDisplayUnit(bool display) { m_disp_unit_flag = display; }

    void setText(const std::wstring& text) { m_str = text; }

    float distance() const;

    float changeUnit(float value) const;

    const Point3f&      posStart() const { return m_start; }
    const Point3f&      posEnd() const { return m_end; }
    const Point3f&      strPosStart() const { return m_str_start; }
    const std::wstring& dispStr() const { return m_str; }
    int                 strSize() const { return m_str_size; }

    void setPosStartEnd(const Point3f& start, const Point3f& end)
    {
        m_start = start;
        m_end   = end;
    }

    bool isInitSrtPos() const;

    void initStrPosStart();
    void reverse();

    virtual void          updateBoundingBox() override;
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const override;

    const TextQuad& textQuad() { return m_str_area; }
    void            setTextQuad(TextQuad& text_quad) { m_str_area = text_quad; }

    void          setTextStartPosAlign(TextAlignment align) { m_text_start_pos_aligin = align; }
    TextAlignment textStartPosAlign() const { return m_text_start_pos_aligin; }

    TextAlign textAlign() const { return m_text_align; }
    void      setTextAlign(TextAlign text_align) { m_text_align = text_align; }

    void  setLineWidth(float line_width) { m_line_width = line_width; }
    float lineWidth() const { return m_line_width; }

    Type dimensionType() const { return m_type; }

    EdgeType edgeType() const { return m_edge_type; }

    void setEdgeType(EdgeType type) { m_edge_type = type; }

    void setDispName(bool disp);
    bool dispName() const { return m_disp_name_flag; }

    void setDispNameLineBreak(bool line_break);
    bool dispNameLineBreak() const { return m_disp_name_flag; }

    void                setName(const std::wstring& name, bool update = true);
    const std::wstring& name() const { return m_name; }

    void setDisplayFlag(bool disp_unit, bool disp_name, bool disp_value, bool update);

    LengthUnit::Unit baseUnit() const { return m_base_unit; }
    LengthUnit::Unit dispUnit() const { return m_disp_unit; }
    int              decimal() const { return m_decimal; }

    void setStartEdgeDisp(bool disp) { m_start_edge_disp = disp; }
    void setEndEdgeDisp(bool disp) { m_end_edge_disp = disp; }

    bool startEdgeDisp() const { return m_start_edge_disp; }
    bool endEdgeDisp() const { return m_end_edge_disp; }

    TextAlign textLastDisplayAlign() const { return m_text_last_disp_align; }
    void      setTextLastDisplayAlign(TextAlign text_align) { m_text_last_disp_align = text_align; }

protected:
    Dimension();
    Dimension(const Dimension& other);
    ~Dimension();

protected:
    Point3f          m_start;
    Point3f          m_end;
    Point3f          m_end_org;
    Point3f          m_str_start;
    std::wstring     m_str_header;
    std::wstring     m_str;
    int              m_decimal = 0;
    int              m_str_size;
    float            m_line_width = 2.0f;
    Type             m_type       = XYZDistance;
    TextAlign        m_text_align = Horizontal;
    EdgeType         m_edge_type  = Circle;
    LengthUnit::Unit m_base_unit  = LengthUnit::Unit::Nanometer;
    LengthUnit::Unit m_disp_unit  = LengthUnit::Unit::Nanometer;
    TextQuad         m_str_area;

    TextAlignment m_text_start_pos_aligin = BottomLeft;
    bool          m_disp_unit_flag        = false;
    bool          m_disp_name_flag        = false;
    bool          m_disp_name_line_break  = true;
    bool          m_disp_value_flag       = true;

    bool m_start_edge_disp = true;
    bool m_end_edge_disp   = true;

    TextAlign m_text_last_disp_align = Horizontal;

    std::wstring m_name;
};

CORE_NAMESPACE_END

#endif    // CORE_DIMENSION_H
