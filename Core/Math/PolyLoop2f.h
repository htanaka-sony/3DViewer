#ifndef CORE_POLYLOOP2F_H
#define CORE_POLYLOOP2F_H

#include "CoreGlobal.h"

#include "BoundingBox2f.h"
#include "Point2f.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT PolyLoop2f {
public:
    PolyLoop2f() {}
    ~PolyLoop2f() {}

    void setLoop();
    void calcArea();
    void calcBBox();

    inline void                 addPoint(const Point2f& point) { m_vertices.emplace_back(point); }
    inline const VecPoint2f&    points() const { return m_vertices; }
    inline const BoundingBox2f& boundingBox() const { return m_bbox; }
    inline double               area() const { return m_area; }

    void addPointCheckSameLine(const Point2f& point, float eps);
    void removeDuplicateEnd(float eps);

    inline int            count() const { return (int)m_vertices.size(); }
    inline const Point2f& point(int index) const { return m_vertices[index]; }
    const Point2f&        clockWisePoint(int index, bool clockwise) const;
    int                   clockWiseIndex(int index, bool clockwise) const;

    bool isPointInLoop(const Point2f& target_point, float eps) const;

    inline bool isOverlap(const PolyLoop2f* other) const
    {
        if (m_bbox.valid() && other->m_bbox.valid()) {
            return m_bbox.isOverlap(other->m_bbox);
        }
        return false;
    }

    inline bool isInclude(const PolyLoop2f* other) const
    {
        if (m_bbox.valid() && other->m_bbox.valid()) {
            return m_bbox.isInclude(other->m_bbox);
        }
        return false;
    }

    bool checkDuplication() const;

    /// 別途Utilityにするとか検討
    /// 点が線分の終点側の延長上か
    static bool isPointOnExtensionLineEnd(const Point2f& start, const Point2f& end, const Point2f& check,
                                          float zero_eps);

    /// 同一直線上の点除去
    static void removePointOnSameLine(VecPoint2f& vertices, float zero_eps);

    /// 自身のループ
    void removePointOnSameLine(float zero_eps);

protected:
    VecPoint2f    m_vertices;
    BoundingBox2f m_bbox;
    double        m_area;
    bool          m_clockwise;
};

template <typename _myData>
class PolyLoop2fWithData : public PolyLoop2f {
public:
    inline void addPointWithData(const Point2f& point, const _myData& data)
    {
        m_vertices.emplace_back(point);
        m_my_data_list.emplace_back(data);
    }
    void           removeDuplicateEndWithData(float eps);
    void           addPointWithDataCheckSameLine(const Point2f& point, const _myData& data, float eps);
    const _myData& clockWiseData(int index, bool clockwise) { return m_my_data_list[clockWiseIndex(index, clockwise)]; }
    void           removePointAndDataOnSameLine(float zero_eps);

protected:
    std::vector<_myData> m_my_data_list;
};

/// 一旦ヘッダに実装書きたくないので分ける。特定用途だけと思うので
/// 他で使うなら再度検討
extern template class PolyLoop2fWithData<int>;

CORE_NAMESPACE_END

#endif    // CORE_POLYLOOP2F_H
