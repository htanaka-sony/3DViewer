cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

project(VoxViewer)

add_library(${PROJECT_NAME} SHARED
    CommonSubClass.cpp
    CommonSubClass.h
    CommonSubFunc.cpp
    CommonSubFunc.h
    CommonSubFunc.ui

    GeometryInputForm.cpp
    GeometryInputForm.h
    GeometryInputForm.ui
    GeometrySubScene.cpp
    GeometrySubScene.h

    LightSetting.ui
    LightSettingDlg.cpp
    LightSettingDlg.h
    MainWindow3DViewer.cpp
    MainWindow3DViewer.h
    MainWindow3DViewer.ui

    MyOpenGLWidget.cpp
    MyOpenGLWidget.h
    qobj3dviewer.cpp
    qobj3dviewer.h
    ReadVoxel.cpp
    ReadVoxel.h
    resources.qrc
    Vox3DForm.cpp
    Vox3DForm.h
    Vox3DForm.ui
    Vox3DFormSub01.cpp
    Vox3DFormSub02readVoxMakeGL.cpp
    Vox3DFormSub03H.cpp
    Vox3DFormSubW.cpp
    VoxViewerGlobal.h
    RangeSlider.h
    RangeSlider.cpp
    ClippingCtrl.h
    ClippingCtrl.cpp
    CustomTabWidget.h
    NonBorderGroupBox.h
    SettingCtrl.h
    SettingCtrl.cpp
    SettingGuiCtrl.h
    SettingGuiCtrl.cpp
    DimensionCtrl.h
    DimensionCtrl.cpp
    MemoryMappedFile.h
    ReadOpt.h
    ReadOpt.cpp
    ResultCtrl.h
    ResultCtrl.cpp
    CsvUtil.cpp
    CsvUtil.h
    SaveColorMap.ui
    SaveColorMapDlg.h
    SaveColorMapDlg.cpp
    ReadOp2.h
    ReadOp2.cpp
    ${LZ4_ROOT_DIR}/lz4.c
    Op2ArraySetting.ui
    Op2ArraySetting.h
    Op2ArraySetting.cpp
    ResultStyleDefaultCombBox.h
    OptPerformanceSetting.h OptPerformanceSetting.cpp OptPerformanceSetting.ui
    ResultOutputDlg.h ResultOutputDlg.cpp ResultOutputDlg.ui
    UtilityCtrl.h
    UtilityCtrl.cpp
    CustomTableWidget.h
    CustomTableWidget.cpp
    CustomCombo.h
    CustomSpin.h
    CustomDoubleSpin.h
    VoxOutputDlg.ui
    VoxOutputDlg.h
    VoxOutputDlg.cpp
    OutputFileThread.h
    OutputFileThread.cpp
    FileExplorerDialog.h
    FileExplorerDialog.cpp
    CustomLineEdit.h
    CustomLineEdit.cpp
    CustomTooltip.h
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VOXVIEWER_LIBRARY
    )


target_include_directories(${PROJECT_NAME} PRIVATE
    ../Core
    ../Render
    ${TINYCOLORMAP_ROOT_DIR}
    ${LZ4_ROOT_DIR}
    ${ZSTD_ROOT_DIR}
    )

target_link_libraries(${PROJECT_NAME} PRIVATE Core Render)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC opengl32)

# 使用するQtのコンポーネントをリストとして宣言
set(QT_COMPONENTS Core Gui OpenGL OpenGLWidgets Widgets)

# Qtライブラリの設定、QtDLLのコピー、PDB生成
setupQtAndPDBMacro(${PROJECT_NAME} dll "${QT_COMPONENTS}")
