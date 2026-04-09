#include "Shape.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(Shape)

Shape::Shape()
{
    m_color[3] = 1.0f;
}

Shape::Shape(const Shape& other)
{
    m_color      = other.m_color;
    m_renderable = other.m_renderable;
}

Shape::~Shape() {}

BoundingBox3f Shape::boundingBox()
{
    if (m_renderable != nullptr) {
        return m_renderable->boundingBox();
    }
    return BoundingBox3f();
}

BoundingBox3f Shape::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible, bool including_text) const
{
    if (m_renderable != nullptr) {
        return m_renderable->calculateBoundingBox(parent_matrix, only_visible, including_text);
    }
    return BoundingBox3f();
}

void Shape::updateBoundingBox()
{
    if (m_renderable != nullptr) {
        m_renderable->updateBoundingBox();
    }
}

void Shape::markBoxDirty(bool /*notify_parent*/)
{
    if (m_renderable != nullptr) {
        m_renderable->markBoxDirty();
    }
}
void Shape::resetBoxDirty()
{
    if (m_renderable != nullptr) {
        m_renderable->resetBoxDirty();
    }
}
bool Shape::isBoxDirty() const
{
    if (m_renderable != nullptr) {
        return m_renderable->isBoxDirty();
    }
    return false;
}

CORE_NAMESPACE_END
