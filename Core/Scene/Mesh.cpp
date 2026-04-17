#include "Scene/Mesh.h"
#include "Scene/RenderMesh.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(Mesh)

Mesh::Mesh()
{
    auto renderable = RenderMesh::createRenderable();
    m_renderable    = renderable.ptr();
}

Mesh::Mesh(const Mesh& other)
{
    m_renderable = other.m_renderable;
}

void Mesh::createBoxShape(const BoundingBox3f& box)
{
    RenderMesh* mesh = (RenderMesh*)m_renderable.ptr();
    mesh->createBoxShape(box);
}

void Mesh::createBoxTaper(const BoundingBox3f& box, float taperDistance)
{
    RenderMesh* mesh = (RenderMesh*)m_renderable.ptr();
    mesh->createBoxTaper(box, taperDistance);
}

CORE_NAMESPACE_END
