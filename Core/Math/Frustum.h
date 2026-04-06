#ifndef CORE_FRUSTUM_H
#define CORE_FRUSTUM_H

#include "CoreGlobal.h"

#include "BoundingBox3f.h"
#include "Matrix4x4f.h"
#include "Planef.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Frustum {
public:
    void setupFrustum(const Matrix4x4f& mvpMatrix);

    bool isBoxInFrustum(const BoundingBox3f& bbox) const;
    bool isBoxInFrustumExceptNearFar(const BoundingBox3f& bbox) const;

    bool isPointInFrustum(const Point3f& point) const;
    bool isPointInFrustumExceptNearFar(const Point3f& point) const;

    void preMulti(const Matrix4x4f& matrix)
    {
        for (int ic = 0; ic < 6; ++ic)
            m_base_planes[ic] = matrix.preMult(m_base_planes[ic]);
    }

private:
    /// 6つの基本平面を格納
    Planef m_base_planes[6];
};

CORE_NAMESPACE_END

#endif    // CORE_FRUSTUM_H
