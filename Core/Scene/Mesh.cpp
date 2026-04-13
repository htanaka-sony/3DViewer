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

    Point3f min_pos = box.min_pos();
    Point3f max_pos = box.max_pos();

    /// Z-
    mesh->appendQuad(min_pos, Point3f(min_pos.x(), max_pos.y(), min_pos.z()),
                     Point3f(max_pos.x(), max_pos.y(), min_pos.z()), Point3f(max_pos.x(), min_pos.y(), min_pos.z()),
                     /*Point3f(0, 0, -1),*/ 0);

    /// Z+
    mesh->appendQuad(Point3f(min_pos.x(), min_pos.y(), max_pos.z()), Point3f(max_pos.x(), min_pos.y(), max_pos.z()),
                     max_pos, Point3f(min_pos.x(), max_pos.y(), max_pos.z()), /*Point3f(0, 0, 1),*/ 0);

    /// X-
    mesh->appendQuad(min_pos, Point3f(min_pos.x(), min_pos.y(), max_pos.z()),
                     Point3f(min_pos.x(), max_pos.y(), max_pos.z()), Point3f(min_pos.x(), max_pos.y(), min_pos.z()),
                     /*Point3f(-1, 0, 0),*/ 0);

    /// X+
    mesh->appendQuad(Point3f(max_pos.x(), min_pos.y(), min_pos.z()), Point3f(max_pos.x(), max_pos.y(), min_pos.z()),
                     max_pos, Point3f(max_pos.x(), min_pos.y(), max_pos.z()), /*Point3f(1, 0, 0),*/ 0);

    /// Y-
    mesh->appendQuad(min_pos, Point3f(max_pos.x(), min_pos.y(), min_pos.z()),
                     Point3f(max_pos.x(), min_pos.y(), max_pos.z()), Point3f(min_pos.x(), min_pos.y(), max_pos.z()),
                     /*Point3f(0, -1, 0),*/ 0);

    /// Y+
    mesh->appendQuad(Point3f(min_pos.x(), max_pos.y(), min_pos.z()), Point3f(min_pos.x(), max_pos.y(), max_pos.z()),
                     max_pos, Point3f(max_pos.x(), max_pos.y(), min_pos.z()), /*Point3f(0, 1, 0),*/ 0);

    /// Wireframe
}

CORE_NAMESPACE_END
