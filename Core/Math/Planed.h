#ifndef CORE_PLANED_H
#define CORE_PLANED_H

#include "CoreGlobal.h"

#include "Point3d.h"
#include "Point4d.h"

CORE_NAMESPACE_BEGIN

class Planed {
public:
    inline Planed() : m_plane(0.0, 0.0, 0.0, 0.0) {}
    inline Planed(const Point3d& normal, const Point3d& point) { setFromNormalAndPoint(normal, point); }
    inline Planed(double a, double b, double c, double d) : m_plane(a, b, c, d) {}

    inline const Point4d& plane() const { return m_plane; }

    inline Point3d normal() const { return Point3d(m_plane[0], m_plane[1], m_plane[2]); }
    inline double  d() const { return m_plane[3]; }

    inline void set(const Point4d& point) { m_plane = point; }

    inline void set(double a, double b, double c, double d)
    {
        m_plane[0] = a;
        m_plane[1] = b;
        m_plane[2] = c;
        m_plane[3] = d;
    }

    inline void setFromNormalAndPoint(const Point3d& normal, const Point3d& point)
    {
        const auto& normalized = normal.normalized();

        m_plane[0] = normalized.x();
        m_plane[1] = normalized.y();
        m_plane[2] = normalized.z();
        m_plane[3] = -normalized * point;
    }

    inline void setFromPoint(const Point3d& point)
    {
        m_plane[3] = -Point3d(m_plane[0], m_plane[1], m_plane[2]) * point;
    }

    inline Point3d origin() const
    {
        return Point3d(-m_plane[0] * m_plane[3], -m_plane[1] * m_plane[3], -m_plane[2] * m_plane[3]);
    }

    /// 距離（表なら正、裏なら負）
    inline float distanceToPoint(const Point3d& point) const
    {
        return m_plane[0] * point.x() + m_plane[1] * point.y() + m_plane[2] * point.z() + m_plane[3];
    }

private:
    Point4d m_plane;    /// 平面方程式の係数 (A, B, C, D)
};

typedef std::vector<Planed>    VecPlaned;
typedef std::vector<VecPlaned> VecVecPlaned;

CORE_NAMESPACE_END

#endif    // CORE_PLANED_H
