#include "Renderable.h"
#include "RenderMesh.h"

CORE_NAMESPACE_BEGIN

RefPtr<RenderableNode> RenderableNode::createRenderableNode(RenderableObject* renderable)
{
    if (!renderable) {
        return nullptr;
    }
    switch (renderable->type()) {
        case RenderableType::RenderMesh:
            return RenderEditableMesh::createRenderable((RenderMesh*)renderable);
    }

    return nullptr;
}

CORE_NAMESPACE_END
