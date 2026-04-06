#ifndef CORE_POINT3D_H
#define CORE_POINT3D_H

#include "CoreGlobal.h"

#include <cmath>
#include <vector>

#ifdef USE_QT
    #include <QVector3D>
#endif

CORE_NAMESPACE_BEGIN

class Point3d {
    friend class BoundingBox3d;

#ifdef USE_QT
public:
    inline Point3d(const QVector3D& vec) : m_x(vec.x()), m_y(vec.y()), m_z(vec.z()) {}
    inline          operator QVector3D() const { return QVector3D(m_x, m_y, m_z); }
    inline Point3d& operator=(const QVector3D& vec)
    {
        m_x = vec.x();
        m_y = vec.y();
        m_z = vec.z();
        return *this;
    }
#endif

public:
    inline Point3d() : m_x(0.0), m_y(0.0), m_z(0.0) {}
    inline Point3d(double ix, double iy, double iz) : m_x(ix), m_y(iy), m_z(iz) {}
    inline Point3d(const double points[3]) : m_x(points[0]), m_y(points[1]), m_z(points[2]) {}

    inline Point3d operator+(const Point3d& p1) const { return Point3d(m_x + p1.m_x, m_y + p1.m_y, m_z + p1.m_z); }

    inline Point3d operator-(const Point3d& p1) const { return Point3d(m_x - p1.m_x, m_y - p1.m_y, m_z - p1.m_z); }

    inline Point3d operator*(double d) const { return Point3d(m_x * d, m_y * d, m_z * d); }

    inline double operator*(const Point3d& p1) const { return (m_x * p1.m_x + m_y * p1.m_y + m_z * p1.m_z); }

    inline Point3d operator/(double d) const { return Point3d(m_x / d, m_y / d, m_z / d); }

    inline Point3d operator^(const Point3d& other) const
    {
        return Point3d(m_y * other.z() - m_z * other.y(), m_z * other.x() - m_x * other.z(),
                       m_x * other.y() - m_y * other.x());
    }

    inline const Point3d& operator+=(const Point3d& p1)
    {
        m_x += p1.m_x;
        m_y += p1.m_y;
        m_z += p1.m_z;
        return *this;
    }

    inline const Point3d& operator-=(const Point3d& p1)
    {
        m_x -= p1.m_x;
        m_y -= p1.m_y;
        m_z -= p1.m_z;
        return *this;
    }

    inline const Point3d& operator*=(double d)
    {
        m_x *= d;
        m_y *= d;
        m_z *= d;
        return *this;
    }

    inline const Point3d& operator/=(double d)
    {
        m_x /= d;
        m_y /= d;
        m_z /= d;
        return *this;
    }

    inline Point3d operator-() const { return Point3d(-m_x, -m_y, -m_z); }

    inline operator const double*() const { return m_point; }
    inline operator double*() { return m_point; }

    inline double  operator[](int nIndex) const { return m_point[nIndex]; }
    inline double& operator[](int nIndex) { return m_point[nIndex]; }

    inline bool operator==(const Point3d& p1) const { return (m_x == p1.m_x && m_y == p1.m_y && m_z == p1.m_z); }
    inline bool operator!=(const Point3d& p1) const { return (m_x != p1.m_x || m_y != p1.m_y || m_z != p1.m_z); }
    inline bool operator<(const Point3d& p1) const
    {
        if (m_x < p1.m_x) return true;
        if (m_x > p1.m_x) return false;
        if (m_y < p1.m_y) return true;
        if (m_y > p1.m_y) return false;
        if (m_z < p1.m_z) return true;
        if (m_z > p1.m_z) return false;
        return false;
    }

    inline void set(double ix, double iy, double iz)
    {
        m_x = ix;
        m_y = iy;
        m_z = iz;
    }

    inline void setX(double ix) { m_x = ix; }

    inline void setY(double iy) { m_y = iy; }

    inline void setZ(double iz) { m_z = iz; }

    inline double x() const { return m_x; }
    inline double y() const { return m_y; }
    inline double z() const { return m_z; }

    inline double length() const { return sqrt(m_x * m_x + m_y * m_y + m_z * m_z); }

    inline double length2() const { return m_x * m_x + m_y * m_y + m_z * m_z; }

    inline double dotProduct(const Point3d& point) const { return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z; }

    inline Point3d crossProduct(const Point3d& point) const
    {
        return Point3d(m_y * point.m_z - m_z * point.m_y, m_z * point.m_x - m_x * point.m_z,
                       m_x * point.m_y - m_y * point.m_x);
    }

    inline bool normalize()
    {
        double dlength = length();
        if (dlength > 0.0) {
            m_x /= dlength;
            m_y /= dlength;
            m_z /= dlength;
            return true;
        }
        return false;
    }

    inline Point3d normalized() const
    {
        Point3d p(*this);
        p.normalize();
        return p;
    }

    inline double distance2(const Point3d& point) const
    {
        return (m_x - point.m_x) * (m_x - point.m_x) + (m_y - point.m_y) * (m_y - point.m_y)
             + (m_z - point.m_z) * (m_z - point.m_z);
    }

protected:
    union {
        struct {
            double m_x, m_y, m_z;
        };
        double m_point[3];
    };
};
typedef std::vector<Point3d>    VecPoint3d;
typedef std::vector<VecPoint3d> VecVePoint3d;

CORE_NAMESPACE_END

#endif    // CORE_POINT3D_H
