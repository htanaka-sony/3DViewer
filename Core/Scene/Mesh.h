#ifndef MESH_H
#define MESH_H

#include "Scene/Shape.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Mesh : public Shape {
    DECLARE_META_OBJECT(Mesh)
protected:
    Mesh();
    Mesh(const Mesh& other);

public:
    void createBoxShape(const BoundingBox3f& box);
    void createBoxRound(const BoundingBox3f& box, float radius, float ratioX, float ratioY, float ratioZ, float tol);
    void createBoxTapper(const BoundingBox3f& box, float taperDistance);
};

CORE_NAMESPACE_END

#endif    // MESH_H
