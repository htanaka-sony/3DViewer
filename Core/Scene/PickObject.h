#ifndef CORE_PICKOBJECT_H
#define CORE_PICKOBJECT_H

#include <memory>

#include "CoreConstants.h"
#include "CoreGlobal.h"

#include "Math/BoundingBox3f.h"
#include "Math/Matrix4x4f.h"
#include "Memory/RefPtr.h"
#include "Memory/Referenced.h"
#include "Node.h"
#include "Object.h"

CORE_NAMESPACE_BEGIN

class Node;

enum class PickType {
    TypeNone = 0,      /// 未設定
    Vertex   = 100,    /// 頂点
    Edge     = 101,    /// Edge
};

#define DECLARE_META_PICKOBJECT(ClassName)                                                                             \
public:                                                                                                                \
    static RefPtr<ClassName> createObject();                                                                           \
    RefPtr<ClassName>        cloneObject() const;

#define DEFINE_META_PICKOBJECT(ClassName)                                                                              \
    RefPtr<ClassName> ClassName::createObject()                                                                        \
    {                                                                                                                  \
        return RefPtr<ClassName>(new ClassName());                                                                     \
    }                                                                                                                  \
    RefPtr<ClassName> ClassName::cloneObject() const                                                                   \
    {                                                                                                                  \
        return RefPtr<ClassName>(new ClassName(*this));                                                                \
    }

class CORE_EXPORT PickObject : public Referenced {
protected:
    PickObject() {}
    PickObject(const PickObject& other) {}
    ~PickObject() {}

public:
    virtual PickType type() const = 0;
};

class CORE_EXPORT PickVertex : public PickObject {
    DECLARE_META_PICKOBJECT(PickVertex)
public:
    void           setVertex(const Point3f& vertex) { m_vertex = vertex; }
    const Point3f& point() const { return m_vertex; }

protected:
    PickVertex() {}
    PickVertex(const PickVertex& other) { m_vertex = other.m_vertex; }
    ~PickVertex() {}

public:
    virtual PickType type() const { return PickType::Vertex; }

protected:
    Point3f m_vertex;
};

class CORE_EXPORT PickEdge : public PickObject {
    DECLARE_META_PICKOBJECT(PickEdge)
public:
    void           setStartVertex(const Point3f& vertex) { m_start_vertex = vertex; }
    void           setEndVertex(const Point3f& vertex) { m_end_vertex = vertex; }
    const Point3f& startVertex() const { return m_start_vertex; }
    const Point3f& endVertex() const { return m_end_vertex; }

    Point3f closestPoint(const Point3f& point) const;

protected:
    PickEdge() {}
    PickEdge(const PickEdge& other)
    {
        m_start_vertex = other.m_start_vertex;
        m_end_vertex   = other.m_end_vertex;
    }
    ~PickEdge() {}

public:
    virtual PickType type() const { return PickType::Edge; }

protected:
    Point3f m_start_vertex;
    Point3f m_end_vertex;
};

class CORE_EXPORT PickData {
public:
    PickData() : m_pick_node(nullptr), m_pick_node_object(nullptr) {}
    PickData(Node* node, Object* object, PickObject* pick_object = nullptr)
        : m_pick_node(node)
        , m_pick_node_object(object)
        , m_pick_object(pick_object)
    {
    }
    Node*       pickNode() { return m_pick_node; }
    Object*     pickNodeObject() { return m_pick_node_object; }
    PickObject* pickObject() { return m_pick_object.ptr(); }

    void     setPickObject(PickObject* pick_object) { m_pick_object = pick_object; }
    PickType type() const
    {
        if (m_pick_object != nullptr)
            return m_pick_object->type();
        else
            return PickType::TypeNone;
    }

    void           setPickPoint(const Point3f& point) { m_pick_point = point; }
    const Point3f& pickPoint() const { return m_pick_point; }

    void           setPickNorm(const Point3f& norm) { m_pick_norm = norm; }
    const Point3f& pickNorm() const { return m_pick_norm; }

protected:
    Node*              m_pick_node;
    Object*            m_pick_node_object;
    RefPtr<PickObject> m_pick_object;
    Point3f            m_pick_point;
    Point3f            m_pick_norm;
};

CORE_NAMESPACE_END

#endif    // CORE_PICKOBJECT_H
