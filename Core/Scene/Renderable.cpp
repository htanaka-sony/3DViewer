#include "Renderable.h"
#include "RenderMesh.h"
#include "RenderNormalMesh.h"

CORE_NAMESPACE_BEGIN

RenderableNode::RenderableNode(const RenderableNode& other) : Renderable(other)
{
    setRenderableObject(other.renderableObject());
}

RenderableNode::RenderableNode(RenderableObject* renderable)
{
    setRenderableObject(renderable);
}

RefPtr<RenderableNode> RenderableNode::createRenderableNode(RenderableObject* renderable)
{
    if (!renderable) {
        return nullptr;
    }
    switch (renderable->type()) {
        case RenderableType::RenderMesh:
            return RenderEditableMesh::createRenderable((RenderMesh*)renderable);
        case RenderableType::RenderNormalMesh:
            return RenderEditableNormalMesh::createRenderable((RenderNormalMesh*)renderable);
    }

    return nullptr;
}

Renderable::Renderable()
{
    m_color[3] = 1.0f;
}

Renderable::Renderable(const Renderable& other)
{
    m_color = other.m_color;
}

CORE_NAMESPACE_END
