#ifndef CORE_POINT3F_H
#define CORE_POINT3F_H

#include "CoreGlobal.h"

#include <cmath>
#include <vector>

#ifdef USE_QT
    #include <QVector3D>
#endif

CORE_NAMESPACE_BEGIN

class Point4f;

class CORE_EXPORT Point3f {
    friend class BoundingBox3f;

#ifdef USE_QT
public:
    inline Point3f(const QVector3D& vec) : m_x(vec.x()), m_y(vec.y()), m_z(vec.z()) {}
    inline          operator QVector3D() const { return QVector3D(m_x, m_y, m_z); }
    inline Point3f& operator=(const QVector3D& vec)
    {
        m_x = vec.x();
        m_y = vec.y();
        m_z = vec.z();
        return *this;
    }
#endif

public:
    inline Point3f() : m_x(0.0f), m_y(0.0f), m_z(0.0f) {}
    constexpr inline Point3f(float ix, float iy, float iz) : m_x(ix), m_y(iy), m_z(iz) {}
    inline Point3f(const float points[3]) : m_x(points[0]), m_y(points[1]), m_z(points[2]) {}
    Point3f(const Point4f& p);

    inline Point3f operator+(const Point3f& p1) const { return Point3f(m_x + p1.m_x, m_y + p1.m_y, m_z + p1.m_z); }

    inline Point3f operator-(const Point3f& p1) const { return Point3f(m_x - p1.m_x, m_y - p1.m_y, m_z - p1.m_z); }

    inline Point3f operator*(float d) const { return Point3f(m_x * d, m_y * d, m_z * d); }

    inline float operator*(const Point3f& p1) const { return (m_x * p1.m_x + m_y * p1.m_y + m_z * p1.m_z); }

    inline Point3f operator/(float d) const { return Point3f(m_x / d, m_y / d, m_z / d); }

    inline Point3f operator^(const Point3f& other) const
    {
        return Point3f(m_y * other.z() - m_z * other.y(), m_z * other.x() - m_x * other.z(),
                       m_x * other.y() - m_y * other.x());
    }

    inline const Point3f& operator+=(const Point3f& p1)
    {
        m_x += p1.m_x;
        m_y += p1.m_y;
        m_z += p1.m_z;
        return *this;
    }

    inline const Point3f& operator-=(const Point3f& p1)
    {
        m_x -= p1.m_x;
        m_y -= p1.m_y;
        m_z -= p1.m_z;
        return *this;
    }

    inline const Point3f& operator*=(float d)
    {
        m_x *= d;
        m_y *= d;
        m_z *= d;
        return *this;
    }

    inline const Point3f& operator/=(float d)
    {
        m_x /= d;
        m_y /= d;
        m_z /= d;
        return *this;
    }

    inline Point3f operator-() const { return Point3f(-m_x, -m_y, -m_z); }

    inline operator const float*() const { return m_point; }
    inline operator float*() { return m_point; }

    inline float  operator[](int nIndex) const { return m_point[nIndex]; }
    inline float& operator[](int nIndex) { return m_point[nIndex]; }

    inline bool operator==(const Point3f& p1) const { return (m_x == p1.m_x && m_y == p1.m_y && m_z == p1.m_z); }
    inline bool operator!=(const Point3f& p1) const { return (m_x != p1.m_x || m_y != p1.m_y || m_z != p1.m_z); }
    inline bool operator<(const Point3f& p1) const
    {
        if (m_x < p1.m_x) return true;
        if (m_x > p1.m_x) return false;
        if (m_y < p1.m_y) return true;
        if (m_y > p1.m_y) return false;
        if (m_z < p1.m_z) return true;
        if (m_z > p1.m_z) return false;
        return false;
    }

    inline void set(float ix, float iy, float iz)
    {
        m_x = ix;
        m_y = iy;
        m_z = iz;
    }

    inline void setX(float ix) { m_x = ix; }

    inline void setY(float iy) { m_y = iy; }

    inline void setZ(float iz) { m_z = iz; }

    inline float x() const { return m_x; }
    inline float y() const { return m_y; }
    inline float z() const { return m_z; }

    inline float length() const { return sqrt(m_x * m_x + m_y * m_y + m_z * m_z); }

    inline float length2() const { return m_x * m_x + m_y * m_y + m_z * m_z; }

    inline float dotProduct(const Point3f& point) const { return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z; }

    inline Point3f crossProduct(const Point3f& point) const
    {
        return Point3f(m_y * point.m_z - m_z * point.m_y, m_z * point.m_x - m_x * point.m_z,
                       m_x * point.m_y - m_y * point.m_x);
    }

    inline bool normalize()
    {
        float dlength = length();
        if (dlength > 0.0f) {
            m_x /= dlength;
            m_y /= dlength;
            m_z /= dlength;
            return true;
        }
        return false;
    }

    inline Point3f normalized() const
    {
        Point3f p(*this);
        p.normalize();
        return p;
    }

    inline float distance2(const Point3f& point) const
    {
        return (m_x - point.m_x) * (m_x - point.m_x) + (m_y - point.m_y) * (m_y - point.m_y)
             + (m_z - point.m_z) * (m_z - point.m_z);
    }

protected:
    union {
        struct {
            float m_x, m_y, m_z;
        };
        float m_point[3];
    };
};
typedef std::vector<Point3f>    VecPoint3f;
typedef std::vector<VecPoint3f> VecVecPoint3f;

/// とりあえずここで宣言
struct Vertexf {
    Point3f m_position;    /// 頂点座標
    Point3f m_normal;      /// 法線

    inline Vertexf() : m_position(Point3f(0, 0, 0)), m_normal(Point3f(0, 0, 0)) {}

    inline Vertexf(const Point3f& v, const Point3f& n) : m_position(v), m_normal(n) {}

    inline Vertexf(float x, float y, float z, float nx, float ny, float nz)
        : m_position(Point3f(x, y, z))
        , m_normal(Point3f(nx, ny, nz))
    {
    }
};

CORE_NAMESPACE_END

#endif    // CORE_POINT3F_H
