#include "Scene/NormalMesh.h"
#include "Scene/RenderNormalMesh.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(NormalMesh)

NormalMesh::NormalMesh()
{
    auto renderable = RenderNormalMesh::createRenderable();
    m_renderable    = renderable.ptr();
}

NormalMesh::NormalMesh(const NormalMesh& other)
{
    m_renderable = other.m_renderable;
}

void NormalMesh::createBoxRound(const BoundingBox3f& box, float radius, float ratioX, float ratioY, float ratioZ,
                                float tol)
{
    RenderNormalMesh* mesh = (RenderNormalMesh*)m_renderable.ptr();
    mesh->createBoxRound(box, radius, ratioX, ratioY, ratioZ, tol);
}

void NormalMesh::createEllipsoid(float radius_x, float radius_y, float radius_z, float tol)
{
    RenderNormalMesh* mesh = (RenderNormalMesh*)m_renderable.ptr();
    mesh->createEllipsoid(radius_x, radius_y, radius_z, tol);
}

void NormalMesh::createEllipticalCylinder(float radius_x, float radius_y, float height, float taper_dist, float tol)
{
    RenderNormalMesh* mesh = (RenderNormalMesh*)m_renderable.ptr();
    mesh->createEllipticalCylinder(radius_x, radius_y, height, taper_dist, tol);
}

CORE_NAMESPACE_END
