#ifndef CORE_BOUNDINGBOX2F_H
#define CORE_BOUNDINGBOX2F_H

#include "CoreGlobal.h"
#include "Point2f.h"

CORE_NAMESPACE_BEGIN

class BoundingBox2f {
public:
    inline BoundingBox2f() : m_min(FLT_MAX, FLT_MAX), m_max(-FLT_MAX, -FLT_MAX) {}

    inline BoundingBox2f(float xmin, float ymin, float xmax, float ymax) : m_min(xmin, ymin), m_max(xmax, ymax) {}

    inline BoundingBox2f(const Point2f& p0, const Point2f& p1)
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
    }

    inline BoundingBox2f(const BoundingBox2f& box)
    {
        m_min = box.m_min;
        m_max = box.m_max;
    }

    inline const BoundingBox2f& operator=(const BoundingBox2f& bbox)
    {
        m_min = bbox.m_min;
        m_max = bbox.m_max;
        return *this;
    }

    inline void init()
    {
        m_min.set(FLT_MAX, FLT_MAX);
        m_max.set(-FLT_MAX, -FLT_MAX);
    }

    inline bool valid() const { return m_max.m_x >= m_min.m_x && m_max.m_y >= m_min.m_y; }

    inline void set(float xmin, float ymin, float xmax, float ymax)
    {
        m_min.set(xmin, ymin);
        m_max.set(xmax, ymax);
    }

    inline void set(const Point2f& min, const Point2f& max)
    {
        m_min = min;
        m_max = max;
    }

    inline const Point2f& min_pos() const { return m_min; }
    inline const Point2f& max_pos() const { return m_max; }

    inline float& xMin() { return m_min.m_x; }
    inline float  xMin() const { return m_min.m_x; }

    inline float& yMin() { return m_min.m_y; }
    inline float  yMin() const { return m_min.m_y; }

    inline float& xMax() { return m_max.m_x; }
    inline float  xMax() const { return m_max.m_x; }

    inline float& yMax() { return m_max.m_y; }
    inline float  yMax() const { return m_max.m_y; }

    inline float getXLength() const { return m_max.m_x - m_min.m_x; }
    inline float getYLength() const { return m_max.m_y - m_min.m_y; }

    inline const Point2f center() const { return (m_min + m_max) * 0.5; }

    inline void expandBy(const Point2f& v)
    {
        if (v.m_x < m_min.m_x) m_min.m_x = v.m_x;
        if (v.m_x > m_max.m_x) m_max.m_x = v.m_x;

        if (v.m_y < m_min.m_y) m_min.m_y = v.m_y;
        if (v.m_y > m_max.m_y) m_max.m_y = v.m_y;
    }

    inline void expandBy(float x, float y)
    {
        if (x < m_min.m_x) m_min.m_x = x;
        if (x > m_max.m_x) m_max.m_x = x;

        if (y < m_min.m_y) m_min.m_y = y;
        if (y > m_max.m_y) m_max.m_y = y;
    }

    void expandBy(const BoundingBox2f& bb)
    {
        if (!bb.valid()) return;

        if (bb.m_min.m_x < m_min.m_x) m_min.m_x = bb.m_min.m_x;
        if (bb.m_max.m_x > m_max.m_x) m_max.m_x = bb.m_max.m_x;

        if (bb.m_min.m_y < m_min.m_y) m_min.m_y = bb.m_min.m_y;
        if (bb.m_max.m_y > m_max.m_y) m_max.m_y = bb.m_max.m_y;
    }

    inline bool contains(const Point2f& v) const
    {
        return valid() && (v.m_x >= m_min.m_x && v.m_x <= m_max.m_x) && (v.m_y >= m_min.m_y && v.m_y <= m_max.m_y);
    }

    inline void setInitPoint(float x, float y)
    {
        m_min.m_x = m_max.m_x = x;
        m_min.m_y = m_max.m_y = y;
    }

    inline void setInitPoint(const Point2f& v0)
    {
        m_min.m_x = m_max.m_x = v0.m_x;
        m_min.m_y = m_max.m_y = v0.m_y;
    }

    inline bool isOverlap(const BoundingBox2f& bbox) const
    {
        if (m_max.m_x < bbox.m_min.m_x || m_min.m_x > bbox.m_max.m_x) return false;
        if (m_max.m_y < bbox.m_min.m_y || m_min.m_y > bbox.m_max.m_y) return false;

        return true;
    }

    inline bool isInclude(const BoundingBox2f& bbox) const
    {
        if (bbox.m_min.m_x < m_min.m_x || bbox.m_max.m_x > m_max.m_x) return false;
        if (bbox.m_min.m_y < m_min.m_y || bbox.m_max.m_y > m_max.m_y) return false;

        return true;
    }

protected:
    Point2f m_min;
    Point2f m_max;
};

CORE_NAMESPACE_END

#endif    // CORE_BOUNDINGBOX2F_H
