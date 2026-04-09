#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "CoreConstants.h"
#include "Math/BoundingBox3f.h"
#include "Math/Matrix4x4f.h"
#include "Memory/RefPtr.h"
#include "Memory/Referenced.h"
#include "MutexWrapper.h"

#include <functional>

CORE_NAMESPACE_BEGIN

class Node;

#define DECLARE_META_RENDERABLE(ClassName)                                                                             \
public:                                                                                                                \
    static RefPtr<ClassName> createRenderable();                                                                       \
    RefPtr<Renderable>       cloneRenderable() const;                                                                  \
    static RenderableType    staticType();                                                                             \
    RenderableType           type() const override;

#define DEFINE_META_RENDERABLE(ClassName)                                                                              \
    RefPtr<ClassName> ClassName::createRenderable()                                                                    \
    {                                                                                                                  \
        return RefPtr<ClassName>(new ClassName());                                                                     \
    }                                                                                                                  \
    RefPtr<Renderable> ClassName::cloneRenderable() const                                                              \
    {                                                                                                                  \
        return RefPtr<Renderable>(new ClassName(*this));                                                               \
    }                                                                                                                  \
    RenderableType ClassName::staticType()                                                                             \
    {                                                                                                                  \
        return RenderableType::ClassName;                                                                              \
    }                                                                                                                  \
    RenderableType ClassName::type() const                                                                             \
    {                                                                                                                  \
        return RenderableType::ClassName;                                                                              \
    }

#define DECLARE_META_RENDERABLENODE(ClassName, BaseClassName)                                                          \
public:                                                                                                                \
    static RefPtr<ClassName> createRenderable(BaseClassName* base);                                                    \
    RefPtr<Renderable>       cloneRenderable() const;                                                                  \
    static RenderableType    staticType();                                                                             \
    RenderableType           type() const override;

#define DEFINE_META_RENDERABLENODE(ClassName, BaseClassName)                                                           \
    RefPtr<ClassName> ClassName::createRenderable(BaseClassName* base)                                                 \
    {                                                                                                                  \
        return RefPtr<ClassName>(new ClassName(base));                                                                 \
    }                                                                                                                  \
    RefPtr<Renderable> ClassName::cloneRenderable() const                                                              \
    {                                                                                                                  \
        return RefPtr<Renderable>(new ClassName(*this));                                                               \
    }                                                                                                                  \
    RenderableType ClassName::staticType()                                                                             \
    {                                                                                                                  \
        return RenderableType::ClassName;                                                                              \
    }                                                                                                                  \
    RenderableType ClassName::type() const                                                                             \
    {                                                                                                                  \
        return RenderableType::ClassName;                                                                              \
    }

class CORE_EXPORT Renderable : public Referenced {
protected:
    Renderable()                        = default;
    Renderable(const Renderable& other) = default;
    ~Renderable() { deleteRenderData(false); }

public:
    virtual RefPtr<Renderable> cloneRenderable() const = 0;
    virtual RenderableType     type() const            = 0;
    virtual BoundingBox3f      boundingBox()
    {
        if (isBoxDirty()) {
            updateBoundingBox();
        }
        return m_bbox;
    }
    virtual void          updateBoundingBox()                             = 0;
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const = 0;    /// 都度計算

    virtual void clearDisplayData() {}

    void markBoxDirty() { m_box_dirty = true; }
    void resetBoxDirty() { m_box_dirty = false; }
    bool isBoxDirty() const { return m_box_dirty; }

    virtual void deleteNode(Node* node) {}    /// Nodeが消えるとき

    void markRenderDirty() { m_render_dirty = true; }
    void resetRenderDirty() { m_render_dirty = false; }
    bool isRenderDirty() const { return m_render_dirty; }

    void setRenderData(void* render_data, std::function<void(void*, bool)> deleter)
    {
        m_render_data         = render_data;
        m_render_data_deleter = deleter;
        resetRenderDirty();
    }

    void deleteRenderData(bool direct_delete)
    {
        /// Render
        if (m_render_data && m_render_data_deleter) {
            m_render_data_deleter(m_render_data, direct_delete);
        }
        m_render_data         = nullptr;
        m_render_data_deleter = nullptr;
        markRenderDirty();
    }
    virtual void* renderData() { return m_render_data; }

protected:
    /// Render Data (Render DLLで生成データ保持・削除用)
    void*                            m_render_data         = nullptr;
    std::function<void(void*, bool)> m_render_data_deleter = nullptr;

    BoundingBox3f m_bbox;
    bool          m_box_dirty    = true;
    bool          m_render_dirty = true;
};

/// Objectが持つ(元の表示データ)
class CORE_EXPORT RenderableObject : public Renderable {
    friend class RenderableNode;

public:
    int useCount() const { return m_use_count; }

protected:
    void addUseCount() const { m_use_count++; }
    void delUseCount() const { m_use_count--; }

    mutable std::atomic<int> m_use_count;    /// 描画で使っているか
};

/// Nodeが持つ(編集した表示データ)
class CORE_EXPORT RenderableNode : public Renderable {
public:
    static RefPtr<RenderableNode> createRenderableNode(RenderableObject* renderable);

    virtual void* renderData()
    {
        if (m_eneble_edit_display) {
            return m_render_data;
        }
        else {
            if (m_renderable_object != nullptr) {
                return m_renderable_object->m_render_data;
            }
        }
        return nullptr;
    }

    bool isEnableEditDisplayData() const { return m_eneble_edit_display; }
    void setEnalbeEditDisplayData(bool enable)
    {
        if (m_renderable_object != nullptr) {
            if (m_eneble_edit_display != enable) {
                if (enable) {
                    m_renderable_object->delUseCount();
                }
                else {
                    m_renderable_object->addUseCount();
                }
            }
        }
        m_eneble_edit_display = enable;
    }

    const RenderableObject* renderableObject() const { return m_renderable_object.ptr(); }
    RenderableObject*       renderableObject() { return m_renderable_object.ptr(); }

protected:
    void setRenderableObject(const RenderableObject* renderable)
    {
        m_renderable_object = renderable;
        if (m_renderable_object != nullptr) {
            if (!m_eneble_edit_display) {
                m_renderable_object->addUseCount();
            }
        }
    }

    RefPtr<RenderableObject> m_renderable_object;
    bool                     m_eneble_edit_display = false;
};

CORE_NAMESPACE_END

#endif    // RENDERABLE_H
