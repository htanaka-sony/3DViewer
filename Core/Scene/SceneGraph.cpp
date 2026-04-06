#include "SceneGraph.h"

CORE_NAMESPACE_BEGIN

RefPtr<SceneGraph> SceneGraph::createSceneGraph()
{
    return new SceneGraph;
}

void SceneGraph::setLengthUnit(LengthUnit& unit)
{
    m_length_unit = unit;
}

const LengthUnit& SceneGraph::lengthUnit()
{
    return m_length_unit;
}

SceneGraph::SceneGraph()
{
    m_root_node = Node::createNode();
    m_root_node->setName(L"Root");
}

SceneGraph::~SceneGraph()
{
    m_root_node.reset();
    m_scene_views.clear();
}

RefPtr<SceneView> SceneGraph::addSceneView()
{
    auto scene_view = SceneView::createSceneView(this);
    m_scene_views.emplace_back(scene_view);
    return scene_view;
}

void SceneGraph::removeSceneView(RefPtr<SceneView>& scene_view)
{
    auto it = std::find(m_scene_views.begin(), m_scene_views.end(), scene_view);
    if (it != m_scene_views.end()) {
        m_scene_views.erase(it);
    }
}

Node* SceneGraph::rootNode()
{
    if (m_temporary_root_node.ptr()) {
        return m_temporary_root_node.ptr();
    }

    return m_root_node.ptr();
}

void SceneGraph::setTemporaryRoot()
{
    m_temporary_root_node = Node::createNode();
    m_temporary_root_node->setName(L"Root");
}

void SceneGraph::resetTemporaryRoot()
{
    if (m_temporary_root_node.ptr()) {
        m_temporary_root_node->removeAllChild(true);
    }
    m_temporary_root_node.reset();
}

CORE_NAMESPACE_END
