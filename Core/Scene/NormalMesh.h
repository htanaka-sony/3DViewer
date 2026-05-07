#ifndef NORMALMESH_H
#define NORMALMESH_H

#include "Scene/Shape.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT NormalMesh : public Shape {
    DECLARE_META_OBJECT(NormalMesh)
protected:
    NormalMesh();
    NormalMesh(const NormalMesh& other);

public:
    void createBoxRound(const BoundingBox3f& box, float radius, float ratioX, float ratioY, float ratioZ, float tol);
    // radius_x / radius_y / radius_z : 各軸方向の半径 (m)
    void createEllipsoid(float radius_x, float radius_y, float radius_z, float tol);
    // radius_x / radius_y : 底面の半径 (m)
    // height              : 柱の高さ (m)
    // taper_dist          : テーパー量 (m)、0 = テーパー無し、正=上部縮小、負=下部縮小
    void createEllipticalCylinder(float radius_x, float radius_y, float height, float taper_dist, float tol);
};

CORE_NAMESPACE_END

#endif    // NORMALMESH_H
