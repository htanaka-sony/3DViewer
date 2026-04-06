#ifndef CORE_POINT4D_H
#define CORE_POINT4D_H

#include "CoreGlobal.h"

#include <float.h>
#include <vector>

#ifdef USE_QT
    #include <QVector4D>
#endif

CORE_NAMESPACE_BEGIN

class Point4d {
#ifdef USE_QT
public:
    inline Point4d(const QVector4D& vec) : m_x(vec.x()), m_y(vec.y()), m_z(vec.z()), m_w(vec.w()) {}
    inline          operator QVector4D() const { return QVector4D(m_x, m_y, m_z, m_w); }
    inline Point4d& operator=(const QVector4D& vec)
    {
        m_x = vec.x();
        m_y = vec.y();
        m_z = vec.z();
        m_w = vec.w();
        return *this;
    }
#endif

public:
    inline Point4d() : m_x(0.0), m_y(0.0), m_z(0.0), m_w(0.0) {}
    inline Point4d(double ix, double iy, double iz, double iw) : m_x(ix), m_y(iy), m_z(iz), m_w(iw) {}
    inline Point4d(const double points[4]) : m_x(points[0]), m_y(points[1]), m_z(points[2]), m_w(points[3]) {}

    inline Point4d operator+(const Point4d& p1) const
    {
        return Point4d(m_x + p1.m_x, m_y + p1.m_y, m_z + p1.m_z, m_w + p1.m_w);
    }

    inline Point4d operator-(const Point4d& p1) const
    {
        return Point4d(m_x - p1.m_x, m_y - p1.m_y, m_z - p1.m_z, m_w - p1.m_w);
    }

    inline Point4d operator*(double d) const { return Point4d(m_x * d, m_y * d, m_z * d, m_w * d); }

    inline double operator*(const Point4d& p1) const
    {
        return (m_x * p1.m_x + m_y * p1.m_y + m_z * p1.m_z + m_w * p1.m_w);
    }

    inline Point4d operator/(double d) const { return Point4d(m_x / d, m_y / d, m_z / d, m_w / d); }

    inline const Point4d& operator+=(const Point4d& p1)
    {
        m_x += p1.m_x;
        m_y += p1.m_y;
        m_z += p1.m_z;
        m_w += p1.m_w;
        return *this;
    }

    inline const Point4d& operator-=(const Point4d& p1)
    {
        m_x -= p1.m_x;
        m_y -= p1.m_y;
        m_z -= p1.m_z;
        m_w -= p1.m_w;
        return *this;
    }

    inline const Point4d& operator*=(double d)
    {
        m_x *= d;
        m_y *= d;
        m_z *= d;
        m_w *= d;
        return *this;
    }

    inline const Point4d& operator/=(double d)
    {
        m_x /= d;
        m_y /= d;
        m_z /= d;
        m_w /= d;
        return *this;
    }

    inline Point4d operator-() const { return Point4d(-m_x, -m_y, -m_z, -m_w); }

    inline operator const double*() const { return m_point; }
    inline operator double*() { return m_point; }

    inline double  operator[](int nIndex) const { return m_point[nIndex]; }
    inline double& operator[](int nIndex) { return m_point[nIndex]; }

    inline bool operator==(const Point4d& p1) const
    {
        return (m_x == p1.m_x && m_y == p1.m_y && m_z == p1.m_z && m_w == p1.m_w);
    }
    inline bool operator!=(const Point4d& p1) const
    {
        return (m_x != p1.m_x || m_y != p1.m_y || m_z != p1.m_z || m_w != p1.m_w);
    }

    inline void set(double ix, double iy, double iz, double iw)
    {
        m_x = ix;
        m_y = iy;
        m_z = iz;
        m_w = iw;
    }

    inline void setX(double ix) { m_x = ix; }
    inline void setY(double iy) { m_y = iy; }
    inline void setZ(double iz) { m_z = iz; }
    inline void setW(double iw) { m_w = iw; }

    inline double x() const { return m_x; }
    inline double y() const { return m_y; }
    inline double z() const { return m_z; }
    inline double w() const { return m_w; }

    inline double length() const { return sqrt(m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w); }

    inline double length2() const { return m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w; }

    inline double dotProduct(const Point4d& point) const
    {
        return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z + m_w * point.m_w;
    }

    inline bool normalize()
    {
        double dlength = length();
        if (dlength > 0.0) {
            m_x /= dlength;
            m_y /= dlength;
            m_z /= dlength;
            m_w /= dlength;
            return true;
        }
        return false;
    }

    inline Point4d normalized() const
    {
        Point4d p(*this);
        p.normalize();
        return p;
    }

    inline double distance2(const Point4d& point) const
    {
        return (m_x - point.m_x) * (m_x - point.m_x) + (m_y - point.m_y) * (m_y - point.m_y)
             + (m_z - point.m_z) * (m_z - point.m_z) + (m_w - point.m_w) * (m_w - point.m_w);
    }

protected:
    union {
        struct {
            double m_x, m_y, m_z, m_w;
        };
        double m_point[4];
    };
};
typedef std::vector<Point4d>    VecPoint4d;
typedef std::vector<VecPoint4d> VecVecPoint4d;

CORE_NAMESPACE_END

#endif    // CORE_POINT4D_H
