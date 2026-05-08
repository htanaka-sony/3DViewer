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

void NormalMesh::createPolygonPrismX(const std::vector<float>& vertices_y, const std::vector<float>& vertices_z,
                                     float height)
{
    RenderNormalMesh* mesh = (RenderNormalMesh*)m_renderable.ptr();
    mesh->createPolygonPrismX(vertices_y, vertices_z, height);
}

void NormalMesh::createPolygonPrismY(const std::vector<float>& vertices_x, const std::vector<float>& vertices_z,
                                     float height)
{
    RenderNormalMesh* mesh = (RenderNormalMesh*)m_renderable.ptr();
    mesh->createPolygonPrismY(vertices_x, vertices_z, height);
}

void NormalMesh::createPolygonPrismZ(const std::vector<float>& vertices_x, const std::vector<float>& vertices_y,
                                     float height)
{
    RenderNormalMesh* mesh = (RenderNormalMesh*)m_renderable.ptr();
    mesh->createPolygonPrismZ(vertices_x, vertices_y, height);
}

void NormalMesh::createAperture(float outer_xlen, float outer_ylen, float z_len, float ap_x_offset, float ap_y_offset,
                                float ap_xlen, float ap_ylen, int round_state, float radius, float ratio_x,
                                float ratio_y, int taper_state, float taper_dist, float tol)
{
    RenderNormalMesh* mesh = (RenderNormalMesh*)m_renderable.ptr();
    mesh->createAperture(outer_xlen, outer_ylen, z_len, ap_x_offset, ap_y_offset, ap_xlen, ap_ylen, round_state, radius,
                         ratio_x, ratio_y, taper_state, taper_dist, tol);
}

CORE_NAMESPACE_END
