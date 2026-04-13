cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

project(Core)

add_library(${PROJECT_NAME} SHARED
    Math/Point3i.h
    Math/Frustum.cpp
    Math/Frustum.h
    Math/BoundingBox3d.h
    Math/Point3d.h
    Math/Matrix4x4f.h
    Math/Point4d.h
    Math/Point3f.h
    Math/Point4f.h
    Math/Matrix4x4f.cpp
    Math/CoreMath.h
    Math/Point.cpp
    Math/BoundingBox3f.h
    Math/Point2i.h
    Math/Planef.h
    Math/Point2f.h
    Math/BoundingBox2f.h
    Math/UnitSystem.h
    Math/UnitSystem.cpp
    Math/SearchPerform.h
    Math/PolyLoop2f.h
    Math/PolyLoop2f.cpp
    Math/Planed.h
    Memory/RefPtr.h
    Memory/Referenced.h
    Memory/Referenced.cpp
    Memory/ControlBlock.h
    Memory/WeakRefPtr.h
    Scene/Node.h
    Scene/Object.h
    Scene/Node.cpp
    Scene/SceneGraph.h
    Scene/SceneView.h
    Scene/SceneGraph.cpp
    Scene/SceneView.cpp
    Scene/Shape.h
    Scene/Shape.cpp
    Scene/Attribute.h
    Scene/Attribute.cpp
    Scene/Voxel.h
    Scene/Voxel.cpp
    Utils/Clipping.h
    Utils/Clipping.cpp
    CoreGlobal.h
    CoreConstants.h
    MutexWrapper.h
    CoreDebug.h
    Utils/Picking.h
    Utils/Picking.cpp
    Scene/Annotation.h
    Scene/Annotation.cpp
    Scene/Dimension.h
    Scene/Dimension.cpp
    Scene/PickObject.h
    Scene/PickObject.cpp
    Math/CoreMath.cpp
    Scene/MultiDimension.h
    Scene/MultiDimension.cpp
    Scene/VoxelScalar.h
    Scene/VoxelScalar.cpp
    Scene/RenderMesh.h
    Scene/RenderMesh.cpp
    Scene/Renderable.h
    Scene/Renderable.cpp
    Scene/Mesh.h
    Scene/Mesh.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EARCUT_ROOT_DIR}
    ${LZ4_ROOT_DIR}
    )

target_compile_definitions(${PROJECT_NAME} PRIVATE
    CORE_LIBRARY
    USE_QT
    )

# Qtに依存させる（インターフェースは用意する）が、基本独立させる（ほかでも使えるようにする）
# 使用するQtのコンポーネントをリストとして宣言
# なるべく最小限にする
set(QT_COMPONENTS Core Gui)

# Qtライブラリの設定、QtDLLのコピー、PDB生成
setupQtAndPDBMacro(${PROJECT_NAME} dll "${QT_COMPONENTS}")
