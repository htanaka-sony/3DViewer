#include "CoreMath.h"

CORE_NAMESPACE_BEGIN

bool closestPoints(const Point3f& p1, const Point3f& d1, const Point3f& p2, const Point3f& d2, Point3f& closest_point1,
                   Point3f& closest_point2, float tol)
{
    Point3f p21   = p2 - p1;
    float   d1d1  = d1 * d1;
    float   d2d2  = d2 * d2;
    float   d1d2  = d1 * d2;
    float   denom = d1d1 * d2d2 - d1d2 * d1d2;

    if (std::abs(denom) < tol) {
        return false;
    }

    float t = (p21 * d1 * d2d2 - p21 * d2 * d1d2) / denom;
    float u = (p21 * d2 * d1d1 - p21 * d1 * d1d2) / denom;

    closest_point1 = p1 + d1 * t;
    closest_point2 = p2 + d2 * u;
    return true;
}

/// 原点と線分の距離(2次元)
float dist2ZeroSegment2D(const float p0[3], const float p1[3], float& t, float out_pos[2])
{
    float w[2]     = {-p0[0], -p0[1]};
    float vec01[2] = {p1[0] - p0[0], p1[1] - p0[1]};

    float vec01_prod = vec01[0] * vec01[0] + vec01[1] * vec01[1];
    if (vec01_prod == 0.0f) {
        t          = 0.0f;
        out_pos[0] = p0[0];
        out_pos[1] = p0[1];
        return (out_pos[0]) * (out_pos[0]) + (out_pos[1]) * (out_pos[1]);
    }

    t = (w[0] * vec01[0] + w[1] * vec01[1]) / vec01_prod;

    if (t <= 0.0f)
        t = 0.0f;
    else if (t >= 1.0f)
        t = 1.0f;

    out_pos[0] = p0[0] + t * vec01[0];
    out_pos[1] = p0[1] + t * vec01[1];

    return (out_pos[0]) * (out_pos[0]) + (out_pos[1]) * (out_pos[1]);
}

Point3f closestPointOnSegment(const Point3f& point, const Point3f& segment_start, const Point3f& segment_end)
{
    Point3f segment_vector = segment_end - segment_start;
    Point3f point_to_start = point - segment_start;

    float segment_length_squared = segment_vector.length2();
    if (segment_length_squared == 0.0f) {
        return segment_start;
    }

    float t = (point_to_start * segment_vector) / segment_length_squared;

    t = std::max(0.0f, std::min(1.0f, t));

    return segment_start + segment_vector * t;
}

Point3f pointOnLine(const Point3f& point, const Point3f& line_start, const Point3f& line_dir)
{
    Point3f point_to_start = point - line_start;

    float t = (point_to_start * line_dir);

    return line_start + line_dir * t;
}

bool isLinear(const Point3f& p1, const Point3f& p2, const Point3f& p3, float eps)
{
    return ((p2 - p1) ^ (p3 - p1)).length2() <= eps * eps;
}

float cosAngleABC(const Point3f& a, const Point3f& b, const Point3f& c)
{
    Point3f vec0 = a - b;
    Point3f vec1 = c - b;

    float length0 = vec0.length();
    if (length0 == 0.0f) {
        return 1.0f;    /// 角度ゼロとみなす
    }
    float length1 = vec1.length();
    if (length1 == 0.0f) {
        return 1.0f;    /// 角度ゼロとみなす
    }

    return (vec0 * vec1) / (length0 * length1);
}

CORE_NAMESPACE_END
