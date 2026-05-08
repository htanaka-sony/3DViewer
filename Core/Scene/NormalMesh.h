#ifndef NORMALMESH_H
#define NORMALMESH_H

#include <vector>
#include "Scene/Shape.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT NormalMesh : public Shape {
    DECLARE_META_OBJECT(NormalMesh)
protected:
    NormalMesh();
    NormalMesh(const NormalMesh& other);

public:
    void createBoxRound(const BoundingBox3f& box, float radius, float ratioX, float ratioY, float ratioZ, float tol);

    void createEllipsoid(float radius_x, float radius_y, float radius_z, float tol);
    void createEllipticalCylinder(float radius_x, float radius_y, float height, float taper_dist, float tol);

    void createPolygonPrismX(const std::vector<float>& vertices_y, const std::vector<float>& vertices_z, float height);
    void createPolygonPrismY(const std::vector<float>& vertices_x, const std::vector<float>& vertices_z, float height);
    void createPolygonPrismZ(const std::vector<float>& vertices_x, const std::vector<float>& vertices_y, float height);

    void createAperture(float outer_xlen, float outer_ylen, float z_len, float ap_x_offset, float ap_y_offset,
                        float ap_xlen, float ap_ylen, int round_state, float radius, float ratio_x, float ratio_y,
                        int taper_state, float taper_dist, float tol);
};

CORE_NAMESPACE_END

#endif    // NORMALMESH_H
