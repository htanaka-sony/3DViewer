#ifndef CORE_BOUNDINGBOX3D_H
#define CORE_BOUNDINGBOX3D_H

#include "CoreGlobal.h"

#include "Point3d.h"

CORE_NAMESPACE_BEGIN

class BoundingBox3d {
public:
    inline BoundingBox3d() : m_min(DBL_MAX, DBL_MAX, DBL_MAX), m_max(-DBL_MAX, -DBL_MAX, -DBL_MAX) {}

    inline BoundingBox3d(double xmin, double ymin, double zmin, double xmax, double ymax, double zmax)
        : m_min(xmin, ymin, zmin)
        , m_max(xmax, ymax, zmax)
    {
    }

    inline BoundingBox3d(const Point3d& p0, const Point3d& p1)
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

    inline BoundingBox3d(const BoundingBox3d& box)
    {
        m_min = box.m_min;
        m_max = box.m_max;
    }

    inline const BoundingBox3d& operator=(const BoundingBox3d& bbox)
    {
        m_min = bbox.m_min;
        m_max = bbox.m_max;
        return *this;
    };

    inline void init()
    {
        m_min.set(DBL_MAX, DBL_MAX, DBL_MAX);
        m_max.set(-DBL_MAX, -DBL_MAX, -DBL_MAX);
    }

    inline bool valid() const { return m_max.m_x >= m_min.m_x && m_max.m_y >= m_min.m_y && m_max.m_z >= m_min.m_z; }

    inline void set(double xmin, double ymin, double zmin, double xmax, double ymax, double zmax)
    {
        m_min.set(xmin, ymin, zmin);
        m_max.set(xmax, ymax, zmax);
    }

    inline void set(const Point3d& min, const Point3d& max)
    {
        m_min = min;
        m_max = max;
    }

    inline const Point3d& min_pos() const { return m_min; }
    inline const Point3d& max_pos() const { return m_max; }

    inline double& xMin() { return m_min.m_x; }
    inline double  xMin() const { return m_min.m_x; }

    inline double& yMin() { return m_min.m_y; }
    inline double  yMin() const { return m_min.m_y; }

    inline double& zMin() { return m_min.m_z; }
    inline double  zMin() const { return m_min.m_z; }

    inline double& xMax() { return m_max.m_x; }
    inline double  xMax() const { return m_max.m_x; }

    inline double& yMax() { return m_max.m_y; }
    inline double  yMax() const { return m_max.m_y; }

    inline double& zMax() { return m_max.m_z; }
    inline double  zMax() const { return m_max.m_z; }

    inline double getXLength() const { return m_max.m_x - m_min.m_x; }
    inline double getYLength() const { return m_max.m_y - m_min.m_y; }
    inline double getZLength() const { return m_max.m_z - m_min.m_z; }

    inline const Point3d center() const { return (m_min + m_max) * 0.5; }

    inline double radius() const { return sqrt(radius2()); }

    inline double radius2() const { return 0.25 * ((m_max - m_min).length2()); }

    inline const Point3d corner(unsigned int pos) const
    {
        return Point3d(pos & 1 ? m_max.m_x : m_min.m_x, pos & 2 ? m_max.m_y : m_min.m_y,
                       pos & 4 ? m_max.m_z : m_min.m_z);
    }

    inline void expandBy(const Point3d& v)
    {
        if (v.m_x < m_min.m_x) m_min.m_x = v.m_x;
        if (v.m_x > m_max.m_x) m_max.m_x = v.m_x;

        if (v.m_y < m_min.m_y) m_min.m_y = v.m_y;
        if (v.m_y > m_max.m_y) m_max.m_y = v.m_y;

        if (v.m_z < m_min.m_z) m_min.m_z = v.m_z;
        if (v.m_z > m_max.m_z) m_max.m_z = v.m_z;
    }

    inline void expandBy(double x, double y, double z)
    {
        if (x < m_min.m_x) m_min.m_x = x;
        if (x > m_max.m_x) m_max.m_x = x;

        if (y < m_min.m_y) m_min.m_y = y;
        if (y > m_max.m_y) m_max.m_y = y;

        if (z < m_min.m_z) m_min.m_z = z;
        if (z > m_max.m_z) m_max.m_z = z;
    }

    void expandBy(const BoundingBox3d& bb)
    {
        if (!bb.valid()) return;

        if (bb.m_min.m_x < m_min.m_x) m_min.m_x = bb.m_min.m_x;
        if (bb.m_max.m_x > m_max.m_x) m_max.m_x = bb.m_max.m_x;

        if (bb.m_min.m_y < m_min.m_y) m_min.m_y = bb.m_min.m_y;
        if (bb.m_max.m_y > m_max.m_y) m_max.m_y = bb.m_max.m_y;

        if (bb.m_min.m_z < m_min.m_z) m_min.m_z = bb.m_min.m_z;
        if (bb.m_max.m_z > m_max.m_z) m_max.m_z = bb.m_max.m_z;
    }

    void expandBy(double width)
    {
        m_min.m_x -= width;
        m_min.m_y -= width;
        m_min.m_z -= width;

        m_max.m_x += width;
        m_max.m_y += width;
        m_max.m_z += width;
    }

    inline bool contains(const Point3d& v) const
    {
        return valid() && (v.m_x >= m_min.m_x && v.m_x <= m_max.m_x) && (v.m_y >= m_min.m_y && v.m_y <= m_max.m_y)
            && (v.m_z >= m_min.m_z && v.m_z <= m_max.m_z);
    }

    inline void setInitPoint(double x, double y, double z)
    {
        m_min.m_x = m_max.m_x = x;
        m_min.m_y = m_max.m_y = y;
        m_min.m_z = m_max.m_z = z;
    }

    inline void setInitPoint(const Point3d& v0)
    {
        m_min.m_x = m_max.m_x = v0.m_x;
        m_min.m_y = m_max.m_y = v0.m_y;
        m_min.m_z = m_max.m_z = v0.m_z;
    }

    inline bool isOverlap(const BoundingBox3d& bbox) const
    {
        if (m_max.m_x < bbox.m_min.m_x || m_min.m_x > bbox.m_max.m_x) return false;
        if (m_max.m_y < bbox.m_min.m_y || m_min.m_y > bbox.m_max.m_y) return false;
        if (m_max.m_z < bbox.m_min.m_z || m_min.m_z > bbox.m_max.m_z) return false;

        return true;
    }

    inline bool isOverlap(const BoundingBox3d& bbox, double expandRange) const
    {
        if (m_max.m_x + expandRange < bbox.m_min.m_x || m_min.m_x > bbox.m_max.m_x + expandRange) return false;
        if (m_max.m_y + expandRange < bbox.m_min.m_y || m_min.m_y > bbox.m_max.m_y + expandRange) return false;
        if (m_max.m_z + expandRange < bbox.m_min.m_z || m_min.m_z > bbox.m_max.m_z + expandRange) return false;

        return true;
    }

    /// 2乗距離
    inline double distance2Boxes(const BoundingBox3d& bbox) const
    {
        double dDist = 0.0;

        /// x方向
        if (m_max.m_x < bbox.m_min.m_x) {
            double d = m_max.m_x - bbox.m_min.m_x;
            dDist += d * d;
        }
        else if (m_min.m_x > bbox.m_max.m_x) {
            double d = m_min.m_x - bbox.m_max.m_x;
            dDist += d * d;
        }

        /// y方向
        if (m_max.m_y < bbox.m_min.m_y) {
            double d = m_max.m_y - bbox.m_min.m_y;
            dDist += d * d;
        }
        else if (m_min.m_y > bbox.m_max.m_y) {
            double d = m_min.m_y - bbox.m_max.m_y;
            dDist += d * d;
        }

        /// z方向
        if (m_max.m_z < bbox.m_min.m_z) {
            double d = m_max.m_z - bbox.m_min.m_z;
            dDist += d * d;
        }
        else if (m_min.m_z > bbox.m_max.m_z) {
            double d = m_min.m_z - bbox.m_max.m_z;
            dDist += d * d;
        }

        return dDist;
    }

protected:
    Point3d m_min;
    Point3d m_max;
};

CORE_NAMESPACE_END

#endif    // CORE_BOUNDINGBOX3D_H
