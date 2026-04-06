#include "PolyLoop2f.h"
#include <set>

CORE_NAMESPACE_BEGIN

void PolyLoop2f::setLoop()
{
    calcArea();
    calcBBox();
}

void PolyLoop2f::calcArea()
{
    double       area       = 0.0;
    unsigned int num_points = m_vertices.size();
    for (unsigned int ic = 0; ic < num_points; ++ic) {
        area += (m_vertices[ic].x() * m_vertices[(ic + 1) % num_points].y()
                 - m_vertices[ic].y() * m_vertices[(ic + 1) % num_points].x());
    }
    if (area > 0) {
        m_clockwise = false;
    }
    else {
        m_clockwise = true;
    }
    m_area = fabs(area) * 0.5;
}

void PolyLoop2f::calcBBox()
{
    m_bbox.init();
    unsigned int num_points = m_vertices.size();
    for (unsigned int ic = 0; ic < num_points; ++ic) {
        m_bbox.expandBy(m_vertices[ic]);
    }
}

void PolyLoop2f::addPointCheckSameLine(const Point2f& point, float eps)
{
    int vertices_size = m_vertices.size();

    if (vertices_size >= 1) {
        /// 同一点除外
        if ((m_vertices[vertices_size - 1] - point).length2() <= eps * eps) {
            return;
        }
    }
    if (vertices_size >= 2) {
        /// 同一直線上を除外(上書き）
        if (isPointOnExtensionLineEnd(m_vertices[vertices_size - 2], m_vertices[vertices_size - 1], point, eps)) {
            m_vertices[vertices_size - 1] = point;
            return;
        }
    }
    m_vertices.emplace_back(point);
}

void PolyLoop2f::removeDuplicateEnd(float eps)
{
    if (m_vertices.size() > 1) {
        if ((m_vertices[m_vertices.size() - 1] - m_vertices[0]).length2() <= eps * eps) {
            m_vertices.pop_back();
        }
    }
}

const Point2f& PolyLoop2f::clockWisePoint(int index, bool clockwise) const
{
    return m_vertices[clockWiseIndex(index, clockwise)];
}

int PolyLoop2f::clockWiseIndex(int index, bool clockwise) const
{
    /// 必ず時計回りに取得する
    if (clockwise) {
        if (m_clockwise)
            return index;
        else
            return count() - 1 - index;
    }
    /// 必ず反時計回りに取得する
    else {
        if (m_clockwise)
            return count() - 1 - index;
        else
            return index;
    }
}

bool PolyLoop2f::isPointInLoop(const Point2f& target_point, float eps) const
{
    int num_points = (int)m_vertices.size();
    if (num_points <= 1) return false;

    int cross_count = 0;

    const Point2f* point0 = &m_vertices[0];
    bool           flag0x = (target_point.x() <= point0->x() - eps);
    bool           flag0y = (target_point.y() <= point0->y() - eps);

    for (int ic = 1; ic < num_points + 1; ++ic) {
        const Point2f* point1 = &m_vertices[ic % num_points];
        bool           flag1x = (target_point.x() <= point1->x() - eps);
        bool           flag1y = (target_point.y() <= point1->y() - eps);
        if (flag0y != flag1y) {
            if (flag0x == flag1x) {
                if (flag0x) {
                    cross_count += (flag0y ? -1 : 1);
                }
            }
            else {
                float edge_x =
                    (point0->x()
                     + (point1->x() - point0->x()) * (target_point.y() - point0->y()) / (point1->y() - point0->y()));
                if (target_point.x() <= edge_x - eps) {
                    cross_count += (flag0y ? -1 : 1);
                }
            }
        }

        point0 = point1;
        flag0x = flag1x;
        flag0y = flag1y;
    }

    return (0 != cross_count);
}

bool PolyLoop2f::checkDuplication() const
{
    if (m_vertices.size() < 1) {
        return false;
    }

    std::set<Point2f> unique_points;
    std::set<Point2f> unique_points_except_start;

    for (int ic = 0; ic < m_vertices.size() - 1; ++ic) {
        if (!unique_points.insert(m_vertices[ic]).second) {
            return true;
        }

        if (ic != 0) {
            unique_points_except_start.insert(m_vertices[ic]);
        }
    }
    const Point2f& end_point = m_vertices.back();
    if (unique_points_except_start.count(end_point)) {
        return true;
    }

    return false;
}

bool PolyLoop2f::isPointOnExtensionLineEnd(const Point2f& start, const Point2f& end, const Point2f& check,
                                           float zero_eps)
{
    const auto& vec1 = end - start;
    const auto& vec2 = check - start;

    float dist1 = vec1.length();
    if (dist1 <= zero_eps) return false;

    float dist2 = vec2.length();
    if (dist2 <= zero_eps) return false;

    /// 延長上にない
    if (dist2 < dist1) return false;

    float prod = (vec1 * vec2) / (dist1 * dist2);

    /// 同一方向
    if (prod >= 1.0 - zero_eps) {
        /// 方向が違う可能性あるのでチェック(zero_eps==0.0fなら不要とは思う)
        const auto& vec3  = check - end;
        float       dist3 = vec3.length();
        if (dist3 < zero_eps) return true;    /// ここはtrue扱いする

        float prod_2 = (vec1 * vec3) / (dist1 * dist3);
        if (prod_2 >= 1.0 - zero_eps) {
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

void PolyLoop2f::removePointOnSameLine(VecPoint2f& vertices, float zero_eps)
{
    float zero_eps_2 = zero_eps * zero_eps;

    unsigned int point_count       = 0;
    unsigned int origin_point_size = vertices.size();
    for (unsigned int ic = 0; ic < origin_point_size; ++ic) {
        if (point_count > 0) {
            /// 同一点も除外
            if ((vertices[point_count - 1] - vertices[ic]).length2() <= zero_eps_2) {
                // vertices[point_count - 1] = vertices[ic];
                continue;
            }
        }

        if (point_count >= 2) {
            /// 同一直線上を除外
            if (isPointOnExtensionLineEnd(vertices[point_count - 2], vertices[point_count - 1], vertices[ic],
                                          zero_eps)) {
                vertices[point_count - 1] = vertices[ic];
                continue;
            }
        }

        /// 除去があれば
        if (point_count != ic) {
            vertices[point_count] = vertices[ic];
        }

        /// カウント
        ++point_count;
    }

    if (point_count > 1) {
        /// 末尾も同一点かどうかをチェックする（微妙なずれの同一点が入っていてPoly2Triが失敗するので）
        if ((vertices[point_count - 1] - vertices[0]).length2() <= zero_eps_2) {
            --point_count;
        }
    }

    if (point_count != vertices.size()) {
        vertices.erase(vertices.begin() + point_count, vertices.end());
    }
}

void PolyLoop2f::removePointOnSameLine(float zero_eps)
{
    removePointOnSameLine(m_vertices, zero_eps);
}

template <typename _myData>
void PolyLoop2fWithData<_myData>::addPointWithDataCheckSameLine(const Point2f& point, const _myData& data, float eps)
{
    int vertices_size = m_vertices.size();

    if (vertices_size >= 1) {
        /// 同一点除外
        if ((m_vertices[vertices_size - 1] - point).length2() <= eps * eps) {
            return;
        }
    }
    if (vertices_size >= 2) {
        /// 同一直線上を除外(上書き）
        if (isPointOnExtensionLineEnd(m_vertices[vertices_size - 2], m_vertices[vertices_size - 1], point, eps)) {
            m_vertices[vertices_size - 1]     = point;
            m_my_data_list[vertices_size - 1] = data;
            return;
        }
    }
    m_vertices.emplace_back(point);
    m_my_data_list.emplace_back(data);
}

template <typename _myData>
void PolyLoop2fWithData<_myData>::removeDuplicateEndWithData(float eps)
{
    if (m_vertices.size() > 1) {
        if ((m_vertices[m_vertices.size() - 1] - m_vertices[0]).length2() <= eps * eps) {
            m_vertices.pop_back();
            m_my_data_list.pop_back();
        }
    }
}

template <typename _myData>
void PolyLoop2fWithData<_myData>::removePointAndDataOnSameLine(float zero_eps)
{
    float zero_eps_2 = zero_eps * zero_eps;

    unsigned int point_count       = 0;
    unsigned int origin_point_size = m_vertices.size();
    for (unsigned int ic = 0; ic < origin_point_size; ++ic) {
        if (point_count > 0) {
            /// 同一点も除外
            if ((m_vertices[point_count - 1] - m_vertices[ic]).length2() <= zero_eps_2) {
                // m_vertices[point_count - 1]     = m_vertices[ic];
                // m_my_data_list[point_count - 1] = m_my_data_list[ic];
                continue;
            }
        }

        if (point_count >= 2) {
            /// 同一直線上を除外
            if (isPointOnExtensionLineEnd(m_vertices[point_count - 2], m_vertices[point_count - 1], m_vertices[ic],
                                          zero_eps)) {
                m_vertices[point_count - 1]     = m_vertices[ic];
                m_my_data_list[point_count - 1] = m_my_data_list[ic];
                continue;
            }
        }

        /// 除去があれば
        if (point_count != ic) {
            m_vertices[point_count]     = m_vertices[ic];
            m_my_data_list[point_count] = m_my_data_list[ic];
        }

        /// カウント
        ++point_count;
    }

    if (point_count > 1) {
        /// 末尾も同一点かどうかをチェックする（微妙なずれの同一点が入っていてPoly2Triが失敗するので）
        if ((m_vertices[point_count - 1] - m_vertices[0]).length2() <= zero_eps_2) {
            --point_count;
        }
    }

    if (point_count != m_vertices.size()) {
        m_vertices.erase(m_vertices.begin() + point_count, m_vertices.end());
        m_my_data_list.erase(m_my_data_list.begin() + point_count, m_my_data_list.end());
    }
}

template class PolyLoop2fWithData<int>;

CORE_NAMESPACE_END
