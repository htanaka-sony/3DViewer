#ifndef CORE_POINT2I_H
#define CORE_POINT2I_H

#include <cmath>
#include <vector>
#include "CoreGlobal.h"

#ifdef USE_QT
    #include <QPoint>
#endif

CORE_NAMESPACE_BEGIN

class Point2i {
#ifdef USE_QT
public:
    inline Point2i(const QPoint& p) : m_x(p.x()), m_y(p.y()) {}
    inline          operator QPoint() const { return QPoint(m_x, m_y); }
    inline Point2i& operator=(const QPoint& p)
    {
        m_x = p.x();
        m_y = p.y();
        return *this;
    }
#endif

public:
    inline Point2i() : m_x(0), m_y(0) {}
    inline Point2i(int ix, int iy) : m_x(ix), m_y(iy) {}
    inline Point2i(const int points[2]) : m_x(points[0]), m_y(points[1]) {}

    inline Point2i operator+(const Point2i& p1) const { return Point2i(m_x + p1.m_x, m_y + p1.m_y); }

    inline Point2i operator-(const Point2i& p1) const { return Point2i(m_x - p1.m_x, m_y - p1.m_y); }

    inline Point2i operator*(int d) const { return Point2i(m_x * d, m_y * d); }
    inline Point2i operator*(float d) const { return Point2i((int)((float)m_x * d), (int)((float)m_y * d)); }
    inline Point2i operator*(double d) const { return Point2i((int)((double)m_x * d), (int)((double)m_y * d)); }
    inline Point2i operator/(int d) const { return Point2i(m_x / d, m_y / d); }

    inline const Point2i& operator+=(const Point2i& p1)
    {
        m_x += p1.m_x;
        m_y += p1.m_y;
        return *this;
    }

    inline const Point2i& operator-=(const Point2i& p1)
    {
        m_x -= p1.m_x;
        m_y -= p1.m_y;
        return *this;
    }

    inline const Point2i& operator*=(int d)
    {
        m_x *= d;
        m_y *= d;
        return *this;
    }

    inline const Point2i& operator/=(int d)
    {
        m_x /= d;
        m_y /= d;
        return *this;
    }

    inline Point2i operator-() const { return Point2i(-m_x, -m_y); }

    inline operator const int*() const { return m_point; }
    inline operator int*() { return m_point; }

    inline int  operator[](int nIndex) const { return m_point[nIndex]; }
    inline int& operator[](int nIndex) { return m_point[nIndex]; }

    inline bool operator==(const Point2i& p1) const { return (m_x == p1.m_x && m_y == p1.m_y); }
    inline bool operator!=(const Point2i& p1) const { return (m_x != p1.m_x || m_y != p1.m_y); }
    inline bool operator<(const Point2i& p1) const
    {
        if (m_x < p1.m_x) return true;
        if (m_x > p1.m_x) return false;
        return m_y < p1.m_y;
    }

    inline void set(int ix, int iy)
    {
        m_x = ix;
        m_y = iy;
    }

    inline void setX(int ix) { m_x = ix; }

    inline void setY(int iy) { m_y = iy; }

    inline int x() const { return m_x; }
    inline int y() const { return m_y; }

    inline double length() const { return sqrt(static_cast<double>(length2())); }

    inline __int64 length2() const { return (__int64)(m_x * m_x) + (__int64)(m_y * m_y); }

    inline int dotProduct(const Point2i& point) const { return m_x * point.m_x + m_y * point.m_y; }

    inline bool normalize()
    {
        double dlength = length();
        if (dlength > 0.0) {
            m_x = static_cast<int>(m_x / dlength);
            m_y = static_cast<int>(m_y / dlength);
            return true;
        }
        return false;
    }

    inline Point2i normalized() const
    {
        Point2i p(*this);
        p.normalize();
        return p;
    }

    inline int distance2(const Point2i& point) const
    {
        return (m_x - point.m_x) * (m_x - point.m_x) + (m_y - point.m_y) * (m_y - point.m_y);
    }

protected:
    union {
        struct {
            int m_x, m_y;
        };
        int m_point[2];
    };
};

typedef std::vector<Point2i>    VecPoint2i;
typedef std::vector<VecPoint2i> VecVecPoint2i;

CORE_NAMESPACE_END

#endif    // CORE_POINT2I_H
