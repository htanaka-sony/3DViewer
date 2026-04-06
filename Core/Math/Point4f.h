#ifndef CORE_POINT4F_H
#define CORE_POINT4F_H

#include "CoreGlobal.h"

#include <cmath>
#include <vector>

#ifdef USE_QT
    #include <QVector4D>
#endif
CORE_NAMESPACE_BEGIN

class Point3f;

class CORE_EXPORT Point4f {
#ifdef USE_QT
public:
    inline Point4f(const QVector4D& vec) : m_x(vec.x()), m_y(vec.y()), m_z(vec.z()), m_w(vec.w()) {}
    inline          operator QVector4D() const { return QVector4D(m_x, m_y, m_z, m_w); }
    inline Point4f& operator=(const QVector4D& vec)
    {
        m_x = vec.x();
        m_y = vec.y();
        m_z = vec.z();
        m_w = vec.w();
        return *this;
    }
#endif

public:
    inline Point4f() : m_x(0.0f), m_y(0.0f), m_z(0.0f), m_w(0.0f) {}
    inline Point4f(float ix, float iy, float iz, float iw) : m_x(ix), m_y(iy), m_z(iz), m_w(iw) {}
    inline Point4f(const float points[4]) : m_x(points[0]), m_y(points[1]), m_z(points[2]), m_w(points[3]) {}
    Point4f(const Point3f& point, float iw);
    Point4f(const Point4f& point) : m_x(point.m_x), m_y(point.m_y), m_z(point.m_z), m_w(point.m_w) {}

    inline Point4f operator+(const Point4f& p1) const
    {
        return Point4f(m_x + p1.m_x, m_y + p1.m_y, m_z + p1.m_z, m_w + p1.m_w);
    }

    inline Point4f operator-(const Point4f& p1) const
    {
        return Point4f(m_x - p1.m_x, m_y - p1.m_y, m_z - p1.m_z, m_w - p1.m_w);
    }

    inline Point4f operator*(float d) const { return Point4f(m_x * d, m_y * d, m_z * d, m_w * d); }

    inline float operator*(const Point4f& p1) const
    {
        return (m_x * p1.m_x + m_y * p1.m_y + m_z * p1.m_z + m_w * p1.m_w);
    }

    inline Point4f operator/(float d) const { return Point4f(m_x / d, m_y / d, m_z / d, m_w / d); }

    inline const Point4f& operator+=(const Point4f& p1)
    {
        m_x += p1.m_x;
        m_y += p1.m_y;
        m_z += p1.m_z;
        m_w += p1.m_w;
        return *this;
    }

    inline const Point4f& operator-=(const Point4f& p1)
    {
        m_x -= p1.m_x;
        m_y -= p1.m_y;
        m_z -= p1.m_z;
        m_w -= p1.m_w;
        return *this;
    }

    inline const Point4f& operator*=(float d)
    {
        m_x *= d;
        m_y *= d;
        m_z *= d;
        m_w *= d;
        return *this;
    }

    inline const Point4f& operator/=(float d)
    {
        m_x /= d;
        m_y /= d;
        m_z /= d;
        m_w /= d;
        return *this;
    }

    inline Point4f operator-() const { return Point4f(-m_x, -m_y, -m_z, -m_w); }

    inline operator const float*() const { return m_point; }
    inline operator float*() { return m_point; }

    inline float  operator[](int nIndex) const { return m_point[nIndex]; }
    inline float& operator[](int nIndex) { return m_point[nIndex]; }

    inline bool operator==(const Point4f& p1) const
    {
        return (m_x == p1.m_x && m_y == p1.m_y && m_z == p1.m_z && m_w == p1.m_w);
    }
    inline bool operator!=(const Point4f& p1) const
    {
        return (m_x != p1.m_x || m_y != p1.m_y || m_z != p1.m_z || m_w != p1.m_w);
    }
    inline bool operator<(const Point4f& p1) const
    {
        if (m_x < p1.m_x) return true;
        if (m_x > p1.m_x) return false;
        if (m_y < p1.m_y) return true;
        if (m_y > p1.m_y) return false;
        if (m_z < p1.m_z) return true;
        if (m_z > p1.m_z) return false;
        if (m_w < p1.m_w) return true;
        if (m_w > p1.m_w) return false;
        return false;
    }

    inline void set(float ix, float iy, float iz, float iw)
    {
        m_x = ix;
        m_y = iy;
        m_z = iz;
        m_w = iw;
    }

    inline void setX(float ix) { m_x = ix; }

    inline void setY(float iy) { m_y = iy; }

    inline void setZ(float iz) { m_z = iz; }

    inline void setW(float iw) { m_w = iw; }

    inline float x() const { return m_x; }
    inline float y() const { return m_y; }
    inline float z() const { return m_z; }
    inline float w() const { return m_w; }

    inline float length() const { return sqrt(m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w); }

    inline float length2() const { return m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w; }

    inline float dotProduct(const Point4f& point) const
    {
        return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z + m_w * point.m_w;
    }

    inline bool normalize()
    {
        float dlength = length();
        if (dlength > 0.0f) {
            m_x /= dlength;
            m_y /= dlength;
            m_z /= dlength;
            m_w /= dlength;
            return true;
        }
        return false;
    }

    inline Point4f normalized() const
    {
        Point4f p(*this);
        p.normalize();
        return p;
    }

    inline float distance2(const Point4f& point) const
    {
        return (m_x - point.m_x) * (m_x - point.m_x) + (m_y - point.m_y) * (m_y - point.m_y)
             + (m_z - point.m_z) * (m_z - point.m_z) + (m_w - point.m_w) * (m_w - point.m_w);
    }

    Point3f toVector3DAffine() const;

protected:
    union {
        struct {
            float m_x, m_y, m_z, m_w;
        };
        float m_point[4];
    };
};
typedef std::vector<Point4f>    VecPoint4f;
typedef std::vector<VecPoint4f> VecVecPoint4f;

CORE_NAMESPACE_END

#endif    // CORE_POINT4F_H
