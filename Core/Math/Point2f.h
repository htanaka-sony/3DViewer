#ifndef CORE_POINT2F_H
#define CORE_POINT2F_H

#include "CoreGlobal.h"

#include <cmath>
#include <vector>

#ifdef USE_QT
    #include <QVector2D>
#endif

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Point2f {
    friend class BoundingBox2f;

#ifdef USE_QT
public:
    inline Point2f(const QVector2D& vec) : m_x(vec.x()), m_y(vec.y()) {}
    inline          operator QVector2D() const { return QVector2D(m_x, m_y); }
    inline Point2f& operator=(const QVector2D& vec)
    {
        m_x = vec.x();
        m_y = vec.y();
        return *this;
    }
#endif

public:
    inline Point2f() : m_x(0.0f), m_y(0.0f) {}
    constexpr inline Point2f(float ix, float iy) : m_x(ix), m_y(iy) {}
    inline Point2f(const float points[2]) : m_x(points[0]), m_y(points[1]) {}

    inline Point2f operator+(const Point2f& p1) const { return Point2f(m_x + p1.m_x, m_y + p1.m_y); }

    inline Point2f operator-(const Point2f& p1) const { return Point2f(m_x - p1.m_x, m_y - p1.m_y); }

    inline Point2f operator*(float d) const { return Point2f(m_x * d, m_y * d); }

    inline float operator*(const Point2f& p1) const { return (m_x * p1.m_x + m_y * p1.m_y); }

    inline float operator^(const Point2f& p1) const { return (m_x * p1.m_y - m_y * p1.m_x); }

    inline Point2f operator/(float d) const { return Point2f(m_x / d, m_y / d); }

    inline const Point2f& operator+=(const Point2f& p1)
    {
        m_x += p1.m_x;
        m_y += p1.m_y;
        return *this;
    }

    inline const Point2f& operator-=(const Point2f& p1)
    {
        m_x -= p1.m_x;
        m_y -= p1.m_y;
        return *this;
    }

    inline const Point2f& operator*=(float d)
    {
        m_x *= d;
        m_y *= d;
        return *this;
    }

    inline const Point2f& operator/=(float d)
    {
        m_x /= d;
        m_y /= d;
        return *this;
    }

    inline Point2f operator-() const { return Point2f(-m_x, -m_y); }

    inline operator const float*() const { return m_point; }
    inline operator float*() { return m_point; }

    inline float  operator[](int nIndex) const { return m_point[nIndex]; }
    inline float& operator[](int nIndex) { return m_point[nIndex]; }

    inline bool operator==(const Point2f& p1) const { return (m_x == p1.m_x && m_y == p1.m_y); }
    inline bool operator!=(const Point2f& p1) const { return (m_x != p1.m_x || m_y != p1.m_y); }
    inline bool operator<(const Point2f& p1) const
    {
        if (m_x < p1.m_x) return true;
        if (m_x > p1.m_x) return false;
        if (m_y < p1.m_y) return true;
        if (m_y > p1.m_y) return false;
        return false;
    }

    inline void set(float ix, float iy)
    {
        m_x = ix;
        m_y = iy;
    }

    inline void setX(float ix) { m_x = ix; }

    inline void setY(float iy) { m_y = iy; }

    inline float x() const { return m_x; }
    inline float y() const { return m_y; }

    inline float length() const { return sqrt(m_x * m_x + m_y * m_y); }

    inline float length2() const { return m_x * m_x + m_y * m_y; }

    inline float dotProduct(const Point2f& point) const { return m_x * point.m_x + m_y * point.m_y; }

    inline bool normalize()
    {
        float dlength = length();
        if (dlength > 0.0f) {
            m_x /= dlength;
            m_y /= dlength;
            return true;
        }
        return false;
    }

    inline Point2f normalized() const
    {
        Point2f p(*this);
        p.normalize();
        return p;
    }

    inline float distance2(const Point2f& point) const
    {
        return (m_x - point.m_x) * (m_x - point.m_x) + (m_y - point.m_y) * (m_y - point.m_y);
    }

protected:
    union {
        struct {
            float m_x, m_y;
        };
        float m_point[2];
    };
};

typedef std::vector<Point2f>    VecPoint2f;
typedef std::vector<VecPoint2f> VecVecPoint2f;

CORE_NAMESPACE_END

#endif    // CORE_POINT2F_H
