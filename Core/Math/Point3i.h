#ifndef CORE_POINT3I_H
#define CORE_POINT3I_H

#include "CoreGlobal.h"

#include <cmath>
#include <vector>

CORE_NAMESPACE_BEGIN

class Point3i {
public:
    inline Point3i() : m_x(0), m_y(0), m_z(0) {}
    inline Point3i(int ix, int iy, int iz) : m_x(ix), m_y(iy), m_z(iz) {}
    inline Point3i(const int points[3]) : m_x(points[0]), m_y(points[1]), m_z(points[2]) {}

    inline Point3i operator+(const Point3i& p1) const { return Point3i(m_x + p1.m_x, m_y + p1.m_y, m_z + p1.m_z); }

    inline Point3i operator-(const Point3i& p1) const { return Point3i(m_x - p1.m_x, m_y - p1.m_y, m_z - p1.m_z); }

    inline Point3i operator*(int d) const { return Point3i(m_x * d, m_y * d, m_z * d); }

    inline Point3i operator/(int d) const { return Point3i(m_x / d, m_y / d, m_z / d); }

    inline const Point3i& operator+=(const Point3i& p1)
    {
        m_x += p1.m_x;
        m_y += p1.m_y;
        m_z += p1.m_z;
        return *this;
    }

    inline const Point3i& operator-=(const Point3i& p1)
    {
        m_x -= p1.m_x;
        m_y -= p1.m_y;
        m_z -= p1.m_z;
        return *this;
    }

    inline const Point3i& operator*=(int d)
    {
        m_x *= d;
        m_y *= d;
        m_z *= d;
        return *this;
    }

    inline const Point3i& operator/=(int d)
    {
        m_x /= d;
        m_y /= d;
        m_z /= d;
        return *this;
    }

    inline Point3i operator-() const { return Point3i(-m_x, -m_y, -m_z); }

    inline operator const int*() const { return m_point; }
    inline operator int*() { return m_point; }

    inline int  operator[](int nIndex) const { return m_point[nIndex]; }
    inline int& operator[](int nIndex) { return m_point[nIndex]; }

    inline bool operator==(const Point3i& p1) const { return (m_x == p1.m_x && m_y == p1.m_y && m_z == p1.m_z); }
    inline bool operator!=(const Point3i& p1) const { return (m_x != p1.m_x || m_y != p1.m_y || m_z != p1.m_z); }
    inline bool operator<(const Point3i& p1) const
    {
        if (m_x < p1.m_x) return true;
        if (m_x > p1.m_x) return false;
        if (m_y < p1.m_y) return true;
        if (m_y > p1.m_y) return false;
        if (m_z < p1.m_z) return true;
        if (m_z > p1.m_z) return false;
        return false;
    }

    inline void set(int ix, int iy, int iz)
    {
        m_x = ix;
        m_y = iy;
        m_z = iz;
    }

    inline void setX(int ix) { m_x = ix; }

    inline void setY(int iy) { m_y = iy; }

    inline void setZ(int iz) { m_z = iz; }

    inline int x() const { return m_x; }
    inline int y() const { return m_y; }
    inline int z() const { return m_z; }

    inline double length() const { return sqrt(static_cast<double>(length2())); }

    inline __int64 length2() const { return (__int64)(m_x * m_x) + (__int64)(m_y * m_y) + (__int64)(m_z * m_z); }

    inline int dotProduct(const Point3i& point) const { return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z; }

    inline Point3i crossProduct(const Point3i& point) const
    {
        return Point3i(m_y * point.m_z - m_z * point.m_y, m_z * point.m_x - m_x * point.m_z,
                       m_x * point.m_y - m_y * point.m_x);
    }

    inline bool normalize()
    {
        double dlength = length();
        if (dlength > 0.0) {
            m_x = static_cast<int>(m_x / dlength);
            m_y = static_cast<int>(m_y / dlength);
            m_z = static_cast<int>(m_z / dlength);
            return true;
        }
        return false;
    }

    inline Point3i normalized() const
    {
        Point3i p(*this);
        p.normalize();
        return p;
    }

    inline int distance2(const Point3i& point) const
    {
        return (m_x - point.m_x) * (m_x - point.m_x) + (m_y - point.m_y) * (m_y - point.m_y)
             + (m_z - point.m_z) * (m_z - point.m_z);
    }

protected:
    union {
        struct {
            int m_x, m_y, m_z;
        };
        int m_point[3];
    };
};
typedef std::vector<Point3i>    VecPoint3i;
typedef std::vector<VecPoint3i> VecVecPoint3i;

CORE_NAMESPACE_END

#endif    // CORE_POINT3I_H
