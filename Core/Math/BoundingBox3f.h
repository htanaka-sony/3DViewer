#ifndef CORE_BOUNDINGBOX3F_H
#define CORE_BOUNDINGBOX3F_H

#include "CoreGlobal.h"
#include "CoreMath.h"

#include "Planef.h"
#include "Point3f.h"

CORE_NAMESPACE_BEGIN

class BoundingBox3f {
public:
    inline BoundingBox3f() : m_min(FLT_MAX, FLT_MAX, FLT_MAX), m_max(-FLT_MAX, -FLT_MAX, -FLT_MAX) {}

    inline BoundingBox3f(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
        : m_min(xmin, ymin, zmin)
        , m_max(xmax, ymax, zmax)
    {
    }

    inline BoundingBox3f(const Point3f& p0, const Point3f& p1)
    {
        if (p0.m_x < p1.m_x) {
            m_min.m_x = p0.m_x;
            m_max.m_x = p1.m_x;
        }
        else {
            m_min.m_x = p1.m_x;
            m_max.m_x = p0.m_x;
        }

        if (p0.m_y < p1.m_y) {
            m_min.m_y = p0.m_y;
            m_max.m_y = p1.m_y;
        }
        else {
            m_min.m_y = p1.m_y;
            m_max.m_y = p0.m_y;
        }

        if (p0.m_z < p1.m_z) {
            m_min.m_z = p0.m_z;
            m_max.m_z = p1.m_z;
        }
        else {
            m_min.m_z = p1.m_z;
            m_max.m_z = p0.m_z;
        }
    }

    inline BoundingBox3f(const BoundingBox3f& box)
    {
        m_min = box.m_min;
        m_max = box.m_max;
    }

    inline const BoundingBox3f& operator=(const BoundingBox3f& bbox)
    {
        m_min = bbox.m_min;
        m_max = bbox.m_max;
        return *this;
    };

    inline void init()
    {
        m_min.set(FLT_MAX, FLT_MAX, FLT_MAX);
        m_max.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    inline bool valid() const { return m_max.m_x >= m_min.m_x && m_max.m_y >= m_min.m_y && m_max.m_z >= m_min.m_z; }

    inline void set(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
    {
        m_min.set(xmin, ymin, zmin);
        m_max.set(xmax, ymax, zmax);
    }

    inline void set(const Point3f& min, const Point3f& max)
    {
        m_min = min;
        m_max = max;
    }

    inline const Point3f& min_pos() const { return m_min; }
    inline const Point3f& max_pos() const { return m_max; }

    inline float& xMin() { return m_min.m_x; }
    inline float  xMin() const { return m_min.m_x; }

    inline float& yMin() { return m_min.m_y; }
    inline float  yMin() const { return m_min.m_y; }

    inline float& zMin() { return m_min.m_z; }
    inline float  zMin() const { return m_min.m_z; }

    inline float& xMax() { return m_max.m_x; }
    inline float  xMax() const { return m_max.m_x; }

    inline float& yMax() { return m_max.m_y; }
    inline float  yMax() const { return m_max.m_y; }

    inline float& zMax() { return m_max.m_z; }
    inline float  zMax() const { return m_max.m_z; }

    inline float getXLength() const { return m_max.m_x - m_min.m_x; }
    inline float getYLength() const { return m_max.m_y - m_min.m_y; }
    inline float getZLength() const { return m_max.m_z - m_min.m_z; }

    inline const Point3f center() const { return (m_min + m_max) * 0.5; }

    inline float radius() const { return sqrt(radius2()); }

    inline float radius2() const { return 0.25 * ((m_max - m_min).length2()); }

    inline const Point3f corner(unsigned int pos) const
    {
        return Point3f(pos & 1 ? m_max.m_x : m_min.m_x, pos & 2 ? m_max.m_y : m_min.m_y,
                       pos & 4 ? m_max.m_z : m_min.m_z);
    }

    inline void expandBy(const Point3f& v)
    {
        if (v.m_x < m_min.m_x) m_min.m_x = v.m_x;
        if (v.m_x > m_max.m_x) m_max.m_x = v.m_x;

        if (v.m_y < m_min.m_y) m_min.m_y = v.m_y;
        if (v.m_y > m_max.m_y) m_max.m_y = v.m_y;

        if (v.m_z < m_min.m_z) m_min.m_z = v.m_z;
        if (v.m_z > m_max.m_z) m_max.m_z = v.m_z;
    }

    inline void expandBy(float x, float y, float z)
    {
        if (x < m_min.m_x) m_min.m_x = x;
        if (x > m_max.m_x) m_max.m_x = x;

        if (y < m_min.m_y) m_min.m_y = y;
        if (y > m_max.m_y) m_max.m_y = y;

        if (z < m_min.m_z) m_min.m_z = z;
        if (z > m_max.m_z) m_max.m_z = z;
    }

    void expandBy(const BoundingBox3f& bb)
    {
        if (!bb.valid()) return;

        if (bb.m_min.m_x < m_min.m_x) m_min.m_x = bb.m_min.m_x;
        if (bb.m_max.m_x > m_max.m_x) m_max.m_x = bb.m_max.m_x;

        if (bb.m_min.m_y < m_min.m_y) m_min.m_y = bb.m_min.m_y;
        if (bb.m_max.m_y > m_max.m_y) m_max.m_y = bb.m_max.m_y;

        if (bb.m_min.m_z < m_min.m_z) m_min.m_z = bb.m_min.m_z;
        if (bb.m_max.m_z > m_max.m_z) m_max.m_z = bb.m_max.m_z;
    }

    void expandBy(float width)
    {
        m_min.m_x -= width;
        m_min.m_y -= width;
        m_min.m_z -= width;

        m_max.m_x += width;
        m_max.m_y += width;
        m_max.m_z += width;
    }

    void shrinkBy(float width)
    {
        m_min.m_x += width;
        m_min.m_y += width;
        m_min.m_z += width;

        m_max.m_x -= width;
        m_max.m_y -= width;
        m_max.m_z -= width;

        if (m_min.m_x > m_max.m_x) {
            m_max.m_x = m_min.m_x;
        }
        if (m_min.m_y > m_max.m_y) {
            m_max.m_y = m_min.m_y;
        }
        if (m_min.m_z > m_max.m_z) {
            m_max.m_z = m_min.m_z;
        }
    }

    void shrinkBy(float x, float y, float z)
    {
        m_min.m_x += x;
        m_min.m_y += y;
        m_min.m_z += z;

        m_max.m_x -= x;
        m_max.m_y -= y;
        m_max.m_z -= z;

        if (m_min.m_x > m_max.m_x) {
            m_max.m_x = m_min.m_x;
        }
        if (m_min.m_y > m_max.m_y) {
            m_max.m_y = m_min.m_y;
        }
        if (m_min.m_z > m_max.m_z) {
            m_max.m_z = m_min.m_z;
        }
    }

    inline bool contains(const Point3f& v) const
    {
        return valid() && (v.m_x >= m_min.m_x && v.m_x <= m_max.m_x) && (v.m_y >= m_min.m_y && v.m_y <= m_max.m_y)
            && (v.m_z >= m_min.m_z && v.m_z <= m_max.m_z);
    }

    inline void setInitPoint(float x, float y, float z)
    {
        m_min.m_x = m_max.m_x = x;
        m_min.m_y = m_max.m_y = y;
        m_min.m_z = m_max.m_z = z;
    }

    inline void setInitPoint(const Point3f& v0)
    {
        m_min.m_x = m_max.m_x = v0.m_x;
        m_min.m_y = m_max.m_y = v0.m_y;
        m_min.m_z = m_max.m_z = v0.m_z;
    }

    inline bool isOverlap(const BoundingBox3f& bbox) const
    {
        if (m_max.m_x < bbox.m_min.m_x || m_min.m_x > bbox.m_max.m_x) return false;
        if (m_max.m_y < bbox.m_min.m_y || m_min.m_y > bbox.m_max.m_y) return false;
        if (m_max.m_z < bbox.m_min.m_z || m_min.m_z > bbox.m_max.m_z) return false;

        return true;
    }

    inline bool isOverlapExceptPlane(const BoundingBox3f& bbox) const
    {
        if (m_max.m_x <= bbox.m_min.m_x || m_min.m_x >= bbox.m_max.m_x) return false;
        if (m_max.m_y <= bbox.m_min.m_y || m_min.m_y >= bbox.m_max.m_y) return false;
        if (m_max.m_z <= bbox.m_min.m_z || m_min.m_z >= bbox.m_max.m_z) return false;

        return true;
    }

    inline bool isOverlap(const BoundingBox3f& bbox, float expandRange) const
    {
        if (m_max.m_x + expandRange < bbox.m_min.m_x || m_min.m_x > bbox.m_max.m_x + expandRange) return false;
        if (m_max.m_y + expandRange < bbox.m_min.m_y || m_min.m_y > bbox.m_max.m_y + expandRange) return false;
        if (m_max.m_z + expandRange < bbox.m_min.m_z || m_min.m_z > bbox.m_max.m_z + expandRange) return false;

        return true;
    }

    inline PlaneOverlap checkOverlapWithPlane(const Planef& plane, float eps = 0) const
    {
        bool positiveSide = false;
        bool negativeSide = false;
        bool onPlane      = false;
        for (int ic = 0; ic < 8; ++ic) {
            float distance = plane.distanceToPoint(corner(ic));
            if (distance > eps) {
                positiveSide = true;
            }
            else if (distance < -eps) {
                negativeSide = true;
            }
            else {
                onPlane = true;
            }
            if (positiveSide && negativeSide) {
                /// 上下に存在
                return PlaneOverlap::Overlapping;
            }
        }
        if (positiveSide) {
            if (onPlane) {
                return PlaneOverlap::AboveOnPlane;    /// 上側＋平面
            }
            else {
                return PlaneOverlap::Above;    /// 上側
            }
        }
        else if (negativeSide) {
            if (onPlane) {
                return PlaneOverlap::BelowOnPlane;    /// 下側＋平面
            }
            else {
                return PlaneOverlap::Below;    /// 下側
            }
        }
        else {
            /// すべて平面上
            return PlaneOverlap::OnPlane;
        }
    }

    /// 2乗距離
    inline float distance2Boxes(const BoundingBox3f& bbox) const
    {
        float dDist = 0.0;

        /// x方向
        if (m_max.m_x < bbox.m_min.m_x) {
            float d = m_max.m_x - bbox.m_min.m_x;
            dDist += d * d;
        }
        else if (m_min.m_x > bbox.m_max.m_x) {
            float d = m_min.m_x - bbox.m_max.m_x;
            dDist += d * d;
        }

        /// y方向
        if (m_max.m_y < bbox.m_min.m_y) {
            float d = m_max.m_y - bbox.m_min.m_y;
            dDist += d * d;
        }
        else if (m_min.m_y > bbox.m_max.m_y) {
            float d = m_min.m_y - bbox.m_max.m_y;
            dDist += d * d;
        }

        /// z方向
        if (m_max.m_z < bbox.m_min.m_z) {
            float d = m_max.m_z - bbox.m_min.m_z;
            dDist += d * d;
        }
        else if (m_min.m_z > bbox.m_max.m_z) {
            float d = m_min.m_z - bbox.m_max.m_z;
            dDist += d * d;
        }

        return dDist;
    }

protected:
    Point3f m_min;
    Point3f m_max;
};

CORE_NAMESPACE_END

#endif    // CORE_BOUNDINGBOX3F_H
