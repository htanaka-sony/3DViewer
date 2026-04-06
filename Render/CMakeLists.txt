cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

project(Render)

add_library(${PROJECT_NAME} SHARED
    RenderGlobal.h
    GLSceneRender.h
    GLSceneRender.cpp
    GLRenderText.cpp
    GLRenderText.h
    resources.qrc
    RenderData.h
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    RENDER_LIBRARY
    )


target_include_directories(${PROJECT_NAME} PRIVATE
    ../Core
    )

target_link_libraries(${PROJECT_NAME} PRIVATE Core)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC opengl32)

# 使用するQtのコンポーネントをリストとして宣言
set(QT_COMPONENTS Core Gui OpenGL OpenGLWidgets Widgets)

# Qtライブラリの設定、QtDLLのコピー、PDB生成
setupQtAndPDBMacro(${PROJECT_NAME} dll "${QT_COMPONENTS}")
