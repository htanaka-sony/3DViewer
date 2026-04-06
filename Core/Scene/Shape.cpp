#include "Shape.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(Shape)

Shape::Shape() : m_box_dirty(true)
{
    m_color[3] = 1.0f;
}

Shape::Shape(const Shape& other) : m_box_dirty(other.m_box_dirty), m_bbox(other.m_bbox) {}

Shape::~Shape() {}

BoundingBox3f Shape::boundingBox()
{
    if (isBoxDirty()) {
        updateBoundingBox();
    }
    return m_bbox;
}

BoundingBox3f Shape::calculateBoundingBox(const Matrix4x4f& /*parent_matrix*/, bool /*only_visible*/,
                                          bool /*including_text*/) const
{
    return BoundingBox3f();
}

void Shape::markBoxDirty(bool /*notify_parent*/)
{
    m_box_dirty = true;
}
void Shape::resetBoxDirty()
{
    m_box_dirty = false;
}
bool Shape::isBoxDirty() const
{
    return m_box_dirty;
}

CORE_NAMESPACE_END
