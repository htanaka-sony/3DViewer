#ifndef CORE_COREMATH_H
#define CORE_COREMATH_H

#include "CoreGlobal.h"
#include "Point3f.h"

//// Define

#ifndef FLT_MAX
    #define FLT_MAX 3.402823466e+38F
#endif

#ifndef M_PI
    #define M_PI (3.14159265358979323846)
#endif

//// Core Define

CORE_NAMESPACE_BEGIN

enum class PlaneOverlap {
    Overlapping,     /// 上下に存在（平面をまたいでいる）
    Above,           /// すべて上側
    Below,           /// すべて下側
    OnPlane,         /// すべて平面上
    AboveOnPlane,    /// すべて上側+平面上
    BelowOnPlane     /// すべて下側+平面上
};

/// Core Function

constexpr inline float degreeToRadian(float degree)
{
    return degree * float(M_PI / 180);
}

constexpr inline double degreeToRadian(double degrees)
{
    return degrees * (M_PI / 180);
}

constexpr inline float radianToDegree(float radian)
{
    return radian * float(180 / M_PI);
}

constexpr inline double radianToDegree(double radian)
{
    return radian * (180 / M_PI);
}

bool CORE_EXPORT closestPoints(const Point3f& p1, const Point3f& d1, const Point3f& p2, const Point3f& d2,
                               Point3f& closest_point1, Point3f& closest_point2, float tol);

Point3f CORE_EXPORT closestPointOnSegment(const Point3f& point, const Point3f& segment_start,
                                          const Point3f& segment_end);

Point3f CORE_EXPORT pointOnLine(const Point3f& point, const Point3f& line_start, const Point3f& line_dir);

float CORE_EXPORT dist2ZeroSegment2D(const float p0[3], const float p1[3], float& t, float out_pos[2]);

bool CORE_EXPORT isLinear(const Point3f& p1, const Point3f& p2, const Point3f& p3, float eps);

float CORE_EXPORT cosAngleABC(const Point3f& a, const Point3f& b, const Point3f& c);

CORE_NAMESPACE_END

#endif    // CORE_COREMATH_H
