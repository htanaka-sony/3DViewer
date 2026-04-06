#include "PickObject.h"
#include "Math/CoreMath.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_PICKOBJECT(PickVertex)

DEFINE_META_PICKOBJECT(PickEdge)

Point3f PickEdge::closestPoint(const Point3f& point) const
{
    return closestPointOnSegment(point, m_start_vertex, m_end_vertex);
}

CORE_NAMESPACE_END
