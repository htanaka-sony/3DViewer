#include "Frustum.h"

CORE_NAMESPACE_BEGIN

void Frustum::setupFrustum(const Matrix4x4f& mvpMatrix)
{
    /// MVP行列から視錐台の平面を抽出
    const float* m = mvpMatrix.ptr();

    /// 左平面
    m_base_planes[0].set(m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]);

    /// 右平面
    m_base_planes[1].set(m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]);

    /// 下平面
    m_base_planes[2].set(m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]);

    /// 上平面
    m_base_planes[3].set(m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]);

    /// 近平面
    m_base_planes[4].set(m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]);

    /// 遠平面
    m_base_planes[5].set(m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]);

    /// 正規化
    for (int ic = 0; ic < 6; ++ic) {
        auto plane_x = m_base_planes[ic].plane().x();
        auto plane_y = m_base_planes[ic].plane().y();
        auto plane_z = m_base_planes[ic].plane().z();
        auto plane_d = m_base_planes[ic].plane().w();

        float length = std::sqrt(plane_x * plane_x + plane_y * plane_y + plane_z * plane_z);
        m_base_planes[ic].set(plane_x / length, plane_y / length, plane_z / length, plane_d / length);
    }
}

bool Frustum::isBoxInFrustum(const BoundingBox3f& bbox) const
{
    for (int ic = 0; ic < 6; ++ic) {
        const auto& plane = m_base_planes[ic];
        if (plane.distanceToPoint(bbox.corner(0)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(1)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(2)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(3)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(4)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(5)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(6)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(7)) > 0) continue;
        return false;
    }
    return true;
}

bool Frustum::isBoxInFrustumExceptNearFar(const BoundingBox3f& bbox) const
{
    for (int ic = 0; ic < 4; ++ic) {
        const auto& plane = m_base_planes[ic];
        if (plane.distanceToPoint(bbox.corner(0)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(1)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(2)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(3)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(4)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(5)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(6)) > 0) continue;
        if (plane.distanceToPoint(bbox.corner(7)) > 0) continue;
        return false;
    }
    return true;
}

bool Frustum::isPointInFrustum(const Point3f& point) const
{
    for (int ic = 0; ic < 6; ++ic) {
        const auto& plane = m_base_planes[ic];
        if (plane.distanceToPoint(point) > 0) continue;
        return false;
    }
    return true;
}

bool Frustum::isPointInFrustumExceptNearFar(const Point3f& point) const
{
    for (int ic = 0; ic < 4; ++ic) {
        const auto& plane = m_base_planes[ic];
        if (plane.distanceToPoint(point) > 0) continue;
        return false;
    }
    return true;
}
CORE_NAMESPACE_END
