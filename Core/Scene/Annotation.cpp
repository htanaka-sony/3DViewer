#include "Annotation.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(Annotation)

Annotation::Annotation() : m_box_dirty(true)
{
    m_color[3] = 1.0f;
}

Annotation::Annotation(const Annotation& other) : m_box_dirty(other.m_box_dirty), m_bbox(other.m_bbox) {}

Annotation::~Annotation() {}

BoundingBox3f Annotation::boundingBox()
{
    if (isBoxDirty()) {
        updateBoundingBox();
    }
    return m_bbox;
}

BoundingBox3f Annotation::calculateBoundingBox(const Matrix4x4f& /*parent_matrix*/, bool /*only_visible*/,
                                               bool /*including_text*/) const
{
    return BoundingBox3f();
}

void Annotation::markBoxDirty(bool /*notify_parent*/)
{
    m_box_dirty = true;
}
void Annotation::resetBoxDirty()
{
    m_box_dirty = false;
}
bool Annotation::isBoxDirty() const
{
    return m_box_dirty;
}

CORE_NAMESPACE_END
