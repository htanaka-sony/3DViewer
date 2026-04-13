#ifndef CORE_OBJECT_H
#define CORE_OBJECT_H

#include <memory>

#include "CoreConstants.h"
#include "CoreGlobal.h"

#include "Math/BoundingBox3f.h"
#include "Math/Matrix4x4f.h"
#include "Memory/RefPtr.h"
#include "Memory/Referenced.h"

CORE_NAMESPACE_BEGIN

class Node;

#define DECLARE_META_OBJECT(ClassName)                                                                                 \
public:                                                                                                                \
    static RefPtr<ClassName> createObject();                                                                           \
    RefPtr<ClassName>        cloneObject() const;                                                                      \
    static ObjectType        staticType();                                                                             \
    ObjectType               type() const override;

#define DEFINE_META_OBJECT(ClassName)                                                                                  \
    RefPtr<ClassName> ClassName::createObject()                                                                        \
    {                                                                                                                  \
        return RefPtr<ClassName>(new ClassName());                                                                     \
    }                                                                                                                  \
    RefPtr<ClassName> ClassName::cloneObject() const                                                                   \
    {                                                                                                                  \
        return RefPtr<ClassName>(new ClassName(*this));                                                                \
    }                                                                                                                  \
    ObjectType ClassName::staticType()                                                                                 \
    {                                                                                                                  \
        return ObjectType::ClassName;                                                                                  \
    }                                                                                                                  \
    ObjectType ClassName::type() const                                                                                 \
    {                                                                                                                  \
        return ObjectType::ClassName;                                                                                  \
    }

class CORE_EXPORT Object : public Referenced {
protected:
    Object()                    = default;
    Object(const Object& other) = default;
    ~Object()                   = default;

public:
    virtual ObjectType    type() const                                    = 0;
    virtual BoundingBox3f boundingBox()                                   = 0;    /// 自身のメンバに設定
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const = 0;    /// 都度計算

    virtual bool isShape() const { return false; }
    virtual bool isVoxel() const { return false; }
    virtual bool isAnnotation() const { return false; }

    virtual void deleteNode(Node* node) {}    /// Nodeが消えるとき
};

CORE_NAMESPACE_END

#endif    // CORE_OBJECT_H
