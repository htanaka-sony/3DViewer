#ifndef CORE_PLANEF_H
#define CORE_PLANEF_H

#include "CoreGlobal.h"

#include "Point3f.h"
#include "Point4f.h"

CORE_NAMESPACE_BEGIN

class Planef {
public:
    inline Planef() : m_plane(0.0f, 0.0f, 0.0f, 0.0f) {}
    inline Planef(const Point3f& normal, const Point3f& point) { setFromNormalAndPoint(normal, point); }
    inline Planef(float a, float b, float c, float d) : m_plane(a, b, c, d) {}

    inline bool operator==(const Planef& p1) const { return (m_plane == p1.m_plane); }

    inline const Point4f& plane() const { return m_plane; }

    inline Point3f normal() const { return Point3f(m_plane[0], m_plane[1], m_plane[2]); }
    inline float   d() const { return m_plane[3]; }

    inline void set(const Point4f& point) { m_plane = point; }

    inline void set(float a, float b, float c, float d)
    {
        m_plane[0] = a;
        m_plane[1] = b;
        m_plane[2] = c;
        m_plane[3] = d;
    }

    inline void setFromNormalAndPoint(const Point3f& normal, const Point3f& point)
    {
        const auto& normalized = normal.normalized();

        m_plane[0] = normalized.x();
        m_plane[1] = normalized.y();
        m_plane[2] = normalized.z();
        m_plane[3] = -normalized * point;
    }

    inline void setFromPoint(const Point3f& point)
    {
        m_plane[3] = -Point3f(m_plane[0], m_plane[1], m_plane[2]) * point;
    }

    inline Point3f origin() const
    {
        return Point3f(-m_plane[0] * m_plane[3], -m_plane[1] * m_plane[3], -m_plane[2] * m_plane[3]);
    }

    /// 距離（表なら正、裏なら負）
    inline float distanceToPoint(const Point3f& point) const
    {
        return m_plane[0] * point.x() + m_plane[1] * point.y() + m_plane[2] * point.z() + m_plane[3];
    }

    inline Point3f projectPoint(const Point3f& point) const
    {
        float distance = distanceToPoint(point);
        return point - normal() * distance;
    }

    /// 線分との交点を計算する関数
    inline bool intersectSegment(const Point3f& p0, const Point3f& p1, Point3f& intersection, bool out_range) const
    {
        Point3f dir   = p1 - p0;
        float   denom = normal() * dir;

        if (std::abs(denom) == 0.0f) {
            /// 線分が平面と平行な場合
            return false;
        }

        float t = -(normal() * p0 + d()) / denom;
        if (!out_range) {
            if ((t < 0.0f) || (t > 1.0f)) {
                return false;
            }
        }

        intersection = p0 + dir * t;
        return true;
    }

private:
    Point4f m_plane;    /// 平面方程式の係数 (A, B, C, D)
};

typedef std::vector<Planef>    VecPlanef;
typedef std::vector<VecPlanef> VecVecPlanef;
CORE_NAMESPACE_END

#endif    // CORE_PLANEF_H
