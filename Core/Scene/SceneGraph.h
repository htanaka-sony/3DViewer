#ifndef CORE_SCENEGRAPH_H
#define CORE_SCENEGRAPH_H

#include "CoreGlobal.h"
#include "Math/UnitSystem.h"
#include "Memory/RefPtr.h"
#include "Memory/Referenced.h"
#include "Node.h"
#include "SceneView.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT SceneGraph : public Referenced {
public:
    static RefPtr<SceneGraph> createSceneGraph();

    void setLengthUnit(LengthUnit& unit);

    const LengthUnit& lengthUnit();

protected:
    SceneGraph();
    ~SceneGraph();

public:
    RefPtr<SceneView> addSceneView();
    void              removeSceneView(RefPtr<SceneView>& scene_view);

    Node* rootNode();

    void setTemporaryRoot();
    void resetTemporaryRoot();

private:
    std::wstring m_name;
    RefPtr<Node> m_root_node;
    RefPtr<Node> m_temporary_root_node;

    VecSceneView m_scene_views;

    LengthUnit m_length_unit;
};

CORE_NAMESPACE_END

#endif    // CORE_SCENEGRAPH_H
