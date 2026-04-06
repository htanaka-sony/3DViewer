#ifndef CORE_ANNOTATION_H
#define CORE_ANNOTATION_H

#include "Object.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Annotation : public Object {
    DECLARE_META_OBJECT(Annotation)
protected:
    Annotation();
    Annotation(const Annotation& other);
    ~Annotation();

public:
    virtual ObjectType    type() const override { return ObjectType::Annotation; }
    virtual BoundingBox3f boundingBox() override;
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const override;

    virtual void updateBoundingBox() {}

    virtual void markBoxDirty(bool notify_parent = true);
    virtual void resetBoxDirty();
    virtual bool isBoxDirty() const;

    virtual void           setColor(const Point3f& color) { setColor(color[0], color[1], color[2]); }
    virtual void           setColor(const Point4f& color) { m_color = color; }
    virtual const Point4f& color() const { return m_color; }

    virtual void setColor(float r, float g, float b)
    {
        m_color.setX(r);
        m_color.setY(g);
        m_color.setZ(b);
    }

    virtual void  setTransparent(float transparent) { m_color.setW(transparent); }
    virtual float transparent() const { return m_color.w(); }

    virtual bool isAnnotation() const { return true; }

protected:
    Point4f       m_color;
    BoundingBox3f m_bbox;
    bool          m_box_dirty;
};

CORE_NAMESPACE_END

#endif    // CORE_ANNOTATION_H
