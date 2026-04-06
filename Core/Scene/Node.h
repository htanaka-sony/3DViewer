#ifndef CORE_NODE_H
#define CORE_NODE_H

#include "CoreGlobal.h"

#include "string.h"

#include "Math/BoundingBox3f.h"
#include "Math/Matrix4x4f.h"

#include "Attribute.h"
#include "Object.h"

#include <algorithm>
#include <set>
#include <vector>

#ifdef USE_QT
    #include <QString>
#endif

CORE_NAMESPACE_BEGIN

class Node;
typedef std::vector<RefPtr<Node>> VecNode;

class Shape;

class CORE_EXPORT Node : public Referenced {
public:
    static RefPtr<Node> createNode();

    const std::wstring& name() const;
    void                setName(const std::wstring& name);
#ifdef USE_QT
    void setName(const QString& name);
#endif

    void  setParent(Node* parent);
    Node* parent() const;

    const VecNode& children() const;
    VecNode&       children();

    int   numChildren();
    Node* child(int index);

    void  appendChild(Node* child);
    Node* addChild();

    void removeChild(Node* node);
    void removeChild(const std::set<Node*>& nodes);
    void removeAllChild(bool recur = false);

    void          setObject(Object* object);
    Object*       object();
    const Object* object() const;

    template <typename T>
    T* object()
    {
        Object* obj = object();
        if (!obj) return nullptr;
        if (obj->type() == T::staticType()) {
            return static_cast<T*>(obj);
        }
        return nullptr;
    }

    template <typename T>
    const T* object() const
    {
        const Object* obj = object();
        if (!obj) return nullptr;
        if (obj->type() == T::staticType()) {
            return static_cast<const T*>(obj);
        }
        return nullptr;
    }

    Shape* shape() const;

    void setBoundingBox(const BoundingBox3f& box);

    void markBoxDirty(bool notify_parent = true);
    void resetBoxDirty();
    bool isBoxDirty() const;

    bool isVisible() const;
    bool isParentVisible() const;

    void show();
    void hide();
    bool isShow() const;

    void setShowHierarchy(bool _show);
    void setShowChildren(bool _show);

    void suppress();
    void unsuppress();
    bool isSuppress() const;

    void setSuppressHierarchy(bool _suppress);
    void setSuppressChildren(bool _suppress);

    void notifyParentBoxDirty();

    const BoundingBox3f& boundingBox();
    BoundingBox3f        displayBoundingBox();

    BoundingBox3f shapeBoundingBox();
    BoundingBox3f shapeDisplayBoundingBox();

    void          updateBoundingBox();
    BoundingBox3f objectBoundingBox();

    /// 正確な取得
    BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix = Matrix4x4f(), bool only_visible = true,
                                       bool including_text = false) const;

    void              setMatrix(const Matrix4x4f& matrix);
    const Matrix4x4f& matrix() const;
    Matrix4x4f        pathMatrix() const;

    Matrix4x4f parentPathMatrix() const;

    void setUserAttributeInt(const std::wstring& key, int value);
    int  userAttributeInt(const std::wstring& key);

    void  setUserAttributeFloat(const std::wstring& key, float value);
    float userAttributeFloat(const std::wstring& key);

    void               setUserAttributeString(const std::wstring& key, const std::wstring& value);
    const std::wstring userAttributeString(const std::wstring& key);

    /// インスタンスコピー生成
    RefPtr<Node> copyInstance();

private:
    Node();
    ~Node();

private:
    std::wstring m_name;

    Node*   m_parent;
    VecNode m_children;

    RefPtr<Object> m_object;

    Matrix4x4f    m_matrix;
    BoundingBox3f m_bbox;

    Attribute<AttributeType> m_attribute;         /// 内部での制御用
    Attribute<std::wstring>  m_attribute_user;    /// 任意の属性

    //// とりあえず素でもつが、bit圧縮検討
    bool m_box_dirty;
    bool m_show;
    bool m_suppress;
};

CORE_NAMESPACE_END

#endif    // CORE_NODE_H
