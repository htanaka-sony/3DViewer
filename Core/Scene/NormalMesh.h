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
};

CORE_NAMESPACE_END

#endif    // NORMALMESH_H
