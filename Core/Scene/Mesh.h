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
};

CORE_NAMESPACE_END

#endif    // MESH_H
