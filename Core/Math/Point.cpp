#include "Point3f.h"
#include "Point4f.h"

CORE_NAMESPACE_BEGIN

Point3f::Point3f(const Point4f& p) : m_x(p.x()), m_y(p.y()), m_z(p.z()) {}

Point4f::Point4f(const Point3f& p, float w) : m_x(p.x()), m_y(p.y()), m_z(p.z()), m_w(w) {}

inline Point3f Point4f::toVector3DAffine() const
{
    if (m_w == 0.0) return Point3f();
    return Point3f(m_x / m_w, m_y / m_w, m_z / m_w);
}

CORE_NAMESPACE_END
