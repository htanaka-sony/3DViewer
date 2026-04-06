#include "MultiDimension.h"
#include "Math/CoreMath.h"
CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(MultiDimension)

void MultiDimension::addDimensions(const std::vector<DimensionInfo>& dimensions)
{
    m_dimensions.insert(m_dimensions.end(), dimensions.begin(), dimensions.end());
}

void MultiDimension::popbackDimensions(int count)
{
    for (int ic = 0; ic < count; ++ic) {
        m_dimensions.pop_back();
    }
}

void MultiDimension::popbackBasePoints(int count)
{
    for (int ic = 0; ic < count; ++ic) {
        m_base_points.pop_back();
    }
}

void MultiDimension::updateBoundingBox()
{
    m_bbox.init();
    for (auto& dimension : m_dimensions) {
        m_bbox.expandBy(dimension.m_dimension->boundingBox());
    }
    if (m_disp_name_flag) {
        if (m_name_dimension.valid()) {
            m_bbox.expandBy(m_name_dimension->boundingBox());
        }
    }
}

BoundingBox3f MultiDimension::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                                   bool including_text) const
{
    BoundingBox3f bbox;
    for (auto& dimension : m_dimensions) {
        bbox.expandBy(dimension.m_dimension->calculateBoundingBox(parent_matrix, only_visible, including_text));
    }
    if (m_disp_name_flag) {
        if (m_name_dimension.valid()) {
            bbox.expandBy(m_name_dimension->calculateBoundingBox(parent_matrix, only_visible, including_text));
        }
    }
    return bbox;
}

void MultiDimension::setColor(float r, float g, float b)
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->setColor(r, g, b);
    }
    if (m_name_dimension.valid()) {
        m_name_dimension->setColor(r, g, b);
    }
    Annotation::setColor(r, g, b);
}

void MultiDimension::setColor(const Point4f& color)
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->setColor(color);
    }
    if (m_name_dimension.valid()) {
        m_name_dimension->setColor(color);
    }
    Annotation::setColor(color);
}

void MultiDimension::setLineWidth(float line_width)
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->setLineWidth(line_width);
    }
    if (m_name_dimension.valid()) {
        m_name_dimension->setLineWidth(line_width);
    }
    m_line_width = line_width;
}

float MultiDimension::lineWidth() const
{
    for (auto& dimension : m_dimensions) {
        return dimension.m_dimension->lineWidth();
    }
    return m_line_width;
}

void MultiDimension::setDisplayUnit(bool display)
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->setDisplayUnit(display);
    }
}

bool MultiDimension::isDisplayUnit()
{
    for (auto& dimension : m_dimensions) {
        return dimension.m_dimension->isDisplayUnit();
    }
    return false;
}

void MultiDimension::setTextAlign(Dimension::TextAlign text_align)
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->setTextAlign(text_align);
    }
    if (m_name_dimension.valid()) {
        m_name_dimension->setTextAlign(text_align);
    }
}

Dimension::TextAlign MultiDimension::textAlign() const
{
    for (auto& dimension : m_dimensions) {
        return dimension.m_dimension->textAlign();
    }
    return Dimension::TextAlign::Horizontal;
}

std::vector<std::wstring> MultiDimension::textList() const
{
    wchar_t                   buf[64];
    std::vector<std::wstring> text_list;

    /// 長さ調整
    size_t max_material_name_size = 0;
    for (auto& dimension : m_dimensions) {
        if (max_material_name_size < dimension.m_material.size()) {
            max_material_name_size = dimension.m_material.size();
        }
    }

    int dimension_count = 1;
    for (auto& dimension : m_dimensions) {
        swprintf_s(buf, sizeof(buf) / sizeof(wchar_t), L"%d", dimension_count++);
        std::wstring str = buf;

        str += L"\t";
        str += dimension.m_material;
        str.append(max_material_name_size - dimension.m_material.size() + 1, ' ');

        str += L"\t";
        str += dimension.m_dimension->createText(false);
        text_list.emplace_back(str);
    }

    return text_list;
}

const std::wstring& MultiDimension::material(int index) const
{
    if (m_dimensions.size() <= index) {
        static std::wstring empty;
        return empty;
    }

    return m_dimensions[index].m_material;
}

const std::wstring& MultiDimension::materialOriginal(int index) const
{
    if (m_dimensions.size() <= index) {
        static std::wstring empty;
        return empty;
    }

    return m_dimensions[index].m_material_original;
}

void MultiDimension::setMaterial(int index, const std::wstring& material)
{
    if (m_dimensions.size() <= index) {
        return;
    }

    m_dimensions[index].m_material = material;

    uptateText(index);
}

void MultiDimension::setMaterial(const std::map<std::wstring, std::wstring>& material_name_change)
{
    for (int ic = 0; ic < m_dimensions.size(); ++ic) {
        auto itr_find = material_name_change.find(m_dimensions[ic].m_material_original);
        if (itr_find != material_name_change.end()) {
            if (m_dimensions[ic].m_material != itr_find->second) {
                m_dimensions[ic].m_material = itr_find->second;
                uptateText(ic);
            }
        }
        else {
            if (m_dimensions[ic].m_material != m_dimensions[ic].m_material_original) {
                m_dimensions[ic].m_material = m_dimensions[ic].m_material_original;
                uptateText(ic);
            }
        }
    }
}

std::wstring MultiDimension::valueString(int index) const
{
    if (m_dimensions.size() <= index) {
        return L"";
    }

    return m_dimensions[index].m_dimension->createText(false);
}

void MultiDimension::setStrSize(int size)
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->setStrSize(size);
    }
    if (m_name_dimension.valid()) {
        m_name_dimension->setStrSize(size);
    }
}

int MultiDimension::strSize()
{
    for (auto& dimension : m_dimensions) {
        return dimension.m_dimension->strSize();
    }
    return 0;
}

void MultiDimension::initStrPosStart()
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->initStrPosStart();
    }
    if (m_name_dimension.valid() && m_dimensions.size() > 0) {
        m_name_dimension->initStrPosStart();
    }
}

void MultiDimension::reverse()
{
    auto temp = m_dimensions;
    m_dimensions.clear();
    for (int ic = temp.size() - 1; ic >= 0; --ic) {
        temp[ic].m_dimension->reverse();
        m_dimensions.emplace_back(temp[ic]);
    }

    auto points = m_base_points;
    m_base_points.clear();
    for (int ic = points.size() - 1; ic >= 0; --ic) {
        m_base_points.emplace_back(points[ic]);
    }

    if (m_name_dimension.valid() && m_dimensions.size() > 0) {
        const Point3f& start = m_dimensions[0].m_dimension->posStart();

        // m_name_dimension->setName(m_name, false);
        m_name_dimension->setDisplayFlag(false, true, false, false);
        m_name_dimension->set(start, start, m_dimensions[0].m_dimension->baseUnit(),
                              m_dimensions[0].m_dimension->dispUnit(), m_dimensions[0].m_dimension->decimal(),
                              m_dimensions[0].m_dimension->strSize());
        m_name_dimension->setStrSize(m_dimensions[0].m_dimension->strSize());
        m_name_dimension->setLineWidth(m_dimensions[0].m_dimension->lineWidth());
        m_name_dimension->setColor(m_dimensions[0].m_dimension->color());
        m_name_dimension->setTextAlign(m_dimensions[0].m_dimension->textAlign());
    }

    updateText();
}

void MultiDimension::setDisplayValue(bool display)
{
    m_display_value = display;
}

void MultiDimension::setDisplayMaterial(bool display)
{
    m_display_material = display;
}

void MultiDimension::setOnSectionPlane(bool on_plane)
{
    m_on_plane = on_plane;
}

Point3f MultiDimension::posStart() const
{
    if (m_dimensions.size() > 0) {
        return m_dimensions[0].m_dimension->posStart();
    }
    return Point3f();
}

Point3f MultiDimension::posEnd() const
{
    if (m_dimensions.size() > 0) {
        return m_dimensions[m_dimensions.size() - 1].m_dimension->posEnd();
    }
    return Point3f();
}

void MultiDimension::updateText()
{
    wchar_t      buf[64];
    std::wstring str;

    int dimension_count = 1;
    for (auto& dimension : m_dimensions) {
        swprintf_s(buf, sizeof(buf) / sizeof(wchar_t), L"<%d>", dimension_count++);
        str = buf;

        if (m_display_material) {
            str += L" ";
            str += dimension.m_material;
        }

        if (m_display_value) {
            str += L" ";
            /// 通常の値のみのテキスト作成
            str += dimension.m_dimension->createText(dimension.m_dimension->isDisplayUnit());
        }

        dimension.m_dimension->setText(str);
    }

    updateEdgeDisp();
}

void MultiDimension::uptateText(int index)
{
    if (index >= m_dimensions.size()) {
        return;
    }

    auto& dimension = m_dimensions[index];

    wchar_t      buf[64];
    std::wstring str;

    swprintf_s(buf, sizeof(buf) / sizeof(wchar_t), L"<%d>", index + 1);
    str = buf;

    if (m_display_material) {
        str += L" ";
        str += dimension.m_material;
    }

    if (m_display_value) {
        str += L" ";
        /// 通常の値のみのテキスト作成
        str += dimension.m_dimension->createText(dimension.m_dimension->isDisplayUnit());
    }

    dimension.m_dimension->setText(str);
}

void MultiDimension::updateEdgeDisp(float /*eps*/)
{
    /// 余計な事せず常に表示でいいか・・・とりあえずフラグは残しておく
    /*
    if (m_dimensions.size() == 0) {
        return;
    }

    auto edge_type = edgeType();
    if (edge_type == Dimension::EdgeType::Circle) {
        for (auto& dimension : m_dimensions) {
            dimension.m_dimension->setStartEdgeDisp(true);
            dimension.m_dimension->setStartEdgeDisp(true);
        }
        return;
    }

    /// 最初と最後は表示
    m_dimensions[0].m_dimension->setStartEdgeDisp(true);
    m_dimensions[m_dimensions.size() - 1].m_dimension->setEndEdgeDisp(true);

    /// 中間
    for (int ic = 1; ic < m_dimensions.size(); ++ic) {
        auto dim_0 = m_dimensions[ic - 1].m_dimension;
        auto dim_1 = m_dimensions[ic].m_dimension;

        /// 折れ線なら端表示しないので判定（●は表示するが、-|は表示しない）

        /// 始点と終点が一致しない
        if (dim_0->posEnd().distance2(dim_1->posStart()) >= eps * eps) {
            /// どちらも表示
            dim_0->setEndEdgeDisp(true);
            dim_1->setStartEdgeDisp(true);
        }
        else {
            /// 始点と終点が一致するが同一直線
            if (isLinear(dim_0->posStart(), dim_0->posEnd(), dim_1->posEnd(), eps)) {
                /// 両方でもいいが片方で
                dim_0->setEndEdgeDisp(true);
                dim_1->setStartEdgeDisp(false);
            }
            /// 始点と終点が一致するが同一直線でない
            else {
                constexpr float COS_135_DEG = -0.7071068f;    // cos(135°)

                /// 角度で判定
                float cos = cosAngleABC(dim_0->posStart(), dim_0->posEnd(), dim_1->posEnd());
                if (cos <= COS_135_DEG) {
                    /// 片方表示
                    dim_0->setEndEdgeDisp(true);
                    dim_1->setStartEdgeDisp(false);
                }
                else {
                    /// 両方表示しない(折れ線なので見た目邪魔）
                    dim_0->setEndEdgeDisp(false);
                    dim_1->setStartEdgeDisp(false);
                }
            }
        }
    }
    */
}

void MultiDimension::createBasePoints(float eps)
{
    /// 線が折れ曲がるところの点を保持
    m_base_points.clear();

    Point3f line_start;
    Point3f line_end;
    for (int ic = 0; ic < m_dimensions.size(); ++ic) {
        auto dimension = m_dimensions[ic].m_dimension;
        if (ic == 0) {
            line_start = dimension->posStart();
            line_end   = dimension->posEnd();
            m_base_points.emplace_back(line_start);
        }
        else {
            if (!isLinear(line_start, line_end, dimension->posStart(), eps)) {
                /// 始点が違う
                m_base_points.emplace_back(line_end);
                m_base_points.emplace_back(dimension->posStart());

                line_start = dimension->posStart();
                line_end   = dimension->posEnd();
            }
            else if (!isLinear(line_start, line_end, dimension->posEnd(), eps)) {
                /// 始点までは同じ、終点が違う
                m_base_points.emplace_back(dimension->posStart());

                line_start = dimension->posStart();
                line_end   = dimension->posEnd();
            }
            else {
                /// 始点も終点も同じ
                line_end = dimension->posEnd();
            }
        }

        if (ic == m_dimensions.size() - 1) {
            m_base_points.emplace_back(dimension->posEnd());
        }
    }
}

bool MultiDimension::removeLastLine(Point3f& last_line_start, Point3f& last_line_end)
{
    int     last_line_index = -1;
    Point3f line_start;
    Point3f line_end;
    for (int ic = m_dimensions.size() - 1; ic >= 0; --ic) {
        const auto& dimension = m_dimensions[ic].m_dimension;
        line_start            = dimension->posStart();
        line_end              = dimension->posEnd();

        if ((line_start - line_end).length2() > 0) {
            last_line_index = ic;
            last_line_start = line_start;
            last_line_end   = line_end;
            break;
        }
    }
    if (last_line_index < 0) {
        m_dimensions.clear();
        return false;
    }

    m_dimensions.erase(m_dimensions.begin() + last_line_index, m_dimensions.end());
    return true;
}

void MultiDimension::createBasePointPlanes(std::vector<OnPlaneInfo>& current_planes, float eps)
{
    m_base_point_planes.clear();
    m_base_point_planes.resize(m_base_points.size());
    for (int ic = 0; ic < m_base_points.size(); ++ic) {
        for (auto& plane : current_planes) {
            if (fabs(plane.m_plane.distanceToPoint(m_base_points[ic])) <= eps) {
                m_base_point_planes[ic].insert(plane.m_clip_index);
            }
        }
    }
}

void MultiDimension::setEdgeType(Dimension::EdgeType edge_type)
{
    for (auto& dimension : m_dimensions) {
        dimension.m_dimension->setEdgeType(edge_type);
    }
    updateEdgeDisp();
}

Dimension::EdgeType MultiDimension::edgeType() const
{
    if (m_dimensions.size() > 0) {
        return m_dimensions[0].m_dimension->edgeType();
    }
    return Dimension::EdgeType::Circle;
}

void MultiDimension::setDispName(bool disp)
{
    m_disp_name_flag = disp;
}

void MultiDimension::setName(const std::wstring& name)
{
    if (m_dimensions.size() == 0) {
        return;
    }

    if (!m_name_dimension.valid()) {
        const Point3f& start = m_dimensions[0].m_dimension->posStart();

        m_name_dimension = Dimension::createObject();
        m_name_dimension->setName(name, false);
        m_name_dimension->setStartEdgeDisp(false);
        m_name_dimension->setEndEdgeDisp(false);
        m_name_dimension->setDisplayFlag(false, true, false, false);
        m_name_dimension->set(start, start, m_dimensions[0].m_dimension->baseUnit(),
                              m_dimensions[0].m_dimension->dispUnit(), m_dimensions[0].m_dimension->decimal(),
                              m_dimensions[0].m_dimension->strSize());
        m_name_dimension->setStrSize(m_dimensions[0].m_dimension->strSize());
        m_name_dimension->setLineWidth(m_dimensions[0].m_dimension->lineWidth());
        m_name_dimension->setColor(m_dimensions[0].m_dimension->color());
        m_name_dimension->setTextAlign(m_dimensions[0].m_dimension->textAlign());
    }
    else {
        m_name_dimension->setName(name, true);
    }
}

std::wstring MultiDimension::name() const
{
    if (m_name_dimension.valid()) {
        return m_name_dimension->name();
    }
    return L"";
}

MultiDimension::MultiDimension() {}

MultiDimension::MultiDimension(const MultiDimension& other) {}

MultiDimension::~MultiDimension() {}

CORE_NAMESPACE_END
