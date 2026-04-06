#include "Dimension.h"

#include <cwchar>

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(Dimension)

Dimension::Dimension() {}

Dimension::Dimension(const Dimension& other) {}

Dimension::~Dimension() {}

void Dimension::set(const Point3f& start, const Point3f& end, LengthUnit::Unit base_unit, LengthUnit::Unit disp_unit,
                    int decimal, int str_size, Type type)
{
    m_start     = start;
    m_end       = end;
    m_end_org   = end;
    m_base_unit = base_unit;
    m_disp_unit = disp_unit;
    m_decimal   = decimal;
    m_str_size  = str_size;
    m_type      = type;

    if (m_type == XDistance) {
        m_end.setY(m_start.y());
        m_end.setZ(m_start.z());
    }
    else if (m_type == YDistance) {
        m_end.setX(m_start.x());
        m_end.setZ(m_start.z());
    }
    else if (m_type == ZDistance) {
        m_end.setX(m_start.x());
        m_end.setY(m_start.y());
    }

    initStrPosStart();

    updateText();
}

void Dimension::setTextPosition(const Point3f& pos)
{
    m_str_start = pos;
}

void Dimension::updateText()
{
    m_str = createText(m_disp_unit_flag, m_disp_name_flag, m_disp_name_line_break, m_disp_value_flag);
}

std::wstring Dimension::createText(bool disp_unit, bool disp_name, bool disp_name_line_break, bool disp_value) const
{
    float dist = changeUnit(distance());

    /// 暫定 - 後でまとめて。。。
    wchar_t format[128];
    if (disp_value) {
        if (disp_unit) {
            if (disp_name) {
                if (disp_name_line_break) {
                    swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"%s\n%s%%.%df %s", m_name.c_str(),
                               m_str_header.c_str(), m_decimal, LengthUnit::unitString(m_disp_unit).c_str());
                }
                else {
                    swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"%s %s%%.%df %s", m_name.c_str(),
                               m_str_header.c_str(), m_decimal, LengthUnit::unitString(m_disp_unit).c_str());
                }
            }
            else {
                swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"%s%%.%df %s", m_str_header.c_str(), m_decimal,
                           LengthUnit::unitString(m_disp_unit).c_str());
            }
        }
        else {
            if (disp_name) {
                if (disp_name_line_break) {
                    swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"%s\n%s%%.%df", m_name.c_str(),
                               m_str_header.c_str(), m_decimal);
                }
                else {
                    swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"%s %s%%.%df", m_name.c_str(),
                               m_str_header.c_str(), m_decimal);
                }
            }
            else {
                swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"%s%%.%df", m_str_header.c_str(), m_decimal);
            }
        }
    }
    else {
        if (disp_name) {
            swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"%s", m_name.c_str());
        }
        else {
            /// 何もなし
            swprintf_s(format, sizeof(format) / sizeof(wchar_t), L"");
        }
    }

    wchar_t buffer[128];
    swprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), format, dist);
    return std::wstring(buffer);
}

float Dimension::distance() const
{
    float dist = 0;
    if (m_type == XYZDistance) {
        dist = (m_start - m_end).length();
    }
    else if (m_type == XDistance) {
        dist = fabs(m_start.x() - m_end.x());
    }
    else if (m_type == YDistance) {
        dist = fabs(m_start.y() - m_end.y());
    }
    else if (m_type == ZDistance) {
        dist = fabs(m_start.z() - m_end.z());
    }
    return dist;
}

float Dimension::changeUnit(float value) const
{
    return LengthUnit::changeUnit(m_base_unit, m_disp_unit, value);
}

bool Dimension::isInitSrtPos() const
{
    return (m_str_start == (m_start + m_end) / 2.0);
}

void Dimension::initStrPosStart()
{
    m_str_start = (m_start + m_end) / 2.0;
}

void Dimension::reverse()
{
    auto temp = m_start;
    m_start   = m_end;
    m_end     = temp;
}

void Dimension::updateBoundingBox()
{
    m_bbox.init();
    m_bbox.expandBy(m_start);
    m_bbox.expandBy(m_end);
    m_bbox.expandBy(m_str_start);
    if (m_str_area.isValid()) {
        for (int ic = 0; ic < 4; ++ic) {
            m_bbox.expandBy(m_str_area.m_pos[ic]);
        }
    }
}

BoundingBox3f Dimension::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                              bool including_text) const
{
    BoundingBox3f bbox;
    bbox.expandBy(parent_matrix * m_start);
    bbox.expandBy(parent_matrix * m_end);
    bbox.expandBy(parent_matrix * m_str_start);
    if (including_text) {
        if (m_str_area.isValid()) {
            for (int ic = 0; ic < 4; ++ic) {
                bbox.expandBy(parent_matrix * m_str_area.m_pos[ic]);
            }
        }
    }
    return bbox;
}

void Dimension::setDispName(bool disp)
{
    m_disp_name_flag = disp;
    updateText();
}

void Dimension::setDispNameLineBreak(bool line_break)
{
    m_disp_name_line_break = line_break;
    updateText();
}

void Dimension::setName(const std::wstring& name, bool update)
{
    m_name = name;
    if (update) {
        updateText();
    }
}

void Dimension::setDisplayFlag(bool disp_unit, bool disp_name, bool disp_value, bool update)
{
    m_disp_unit_flag  = disp_unit;
    m_disp_name_flag  = disp_name;
    m_disp_value_flag = disp_value;
    if (update) updateText();
}

CORE_NAMESPACE_END
