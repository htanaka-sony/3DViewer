cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF) # OFFにしてsetup_uiで対応する（差分ビルドできないため）（階層分けたら差分ビルドできたが、AUTOUICが階層またがってエラーになるのでOFFにしてスクリプトで対応）
set(CMAKE_AUTORCC ON)

project(VoxViewer)

add_library(${PROJECT_NAME} SHARED
    CommonSubClass.cpp
    CommonSubClass.h
    ui_files/CommonSubFunc.cpp
    ui_files/CommonSubFunc.h
    ui_files/CommonSubFunc.ui

    ui_files/GeometryInputForm.cpp
    ui_files/GeometryInputForm.h
    ui_files/GeometryInputForm.ui
    GeometrySubScene.cpp
    GeometrySubScene.h

    ui_files/LightSetting.ui
    ui_files/LightSettingDlg.cpp
    ui_files/LightSettingDlg.h
    ui_files/MainWindow3DViewer.cpp
    ui_files/MainWindow3DViewer.h
    ui_files/MainWindow3DViewer.ui

    MyOpenGLWidget.cpp
    MyOpenGLWidget.h
    qobj3dviewer.cpp
    qobj3dviewer.h
    ReadVoxel.cpp
    ReadVoxel.h
    resources.qrc
    ui_files/Vox3DForm.cpp
    ui_files/Vox3DForm.h
    ui_files/Vox3DForm.ui
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
    ui_files/SaveColorMap.ui
    ui_files/SaveColorMapDlg.h
    ui_files/SaveColorMapDlg.cpp
    ReadOp2.h
    ReadOp2.cpp
    ${LZ4_ROOT_DIR}/lz4.c
    ui_files/Op2ArraySetting.ui
    ui_files/Op2ArraySetting.h
    ui_files/Op2ArraySetting.cpp
    ResultStyleDefaultCombBox.h
    ui_files/OptPerformanceSetting.h
    ui_files/OptPerformanceSetting.cpp
    ui_files/OptPerformanceSetting.ui
    ui_files/ResultOutputDlg.h
    ui_files/ResultOutputDlg.cpp
    ui_files/ResultOutputDlg.ui
    UtilityCtrl.h
    UtilityCtrl.cpp
    CustomTableWidget.h
    CustomTableWidget.cpp
    CustomCombo.h
    CustomSpin.h
    CustomDoubleSpin.h
    ui_files/VoxOutputDlg.ui
    ui_files/VoxOutputDlg.h
    ui_files/VoxOutputDlg.cpp
    OutputFileThread.h
    OutputFileThread.cpp
    FileExplorerDialog.h
    FileExplorerDialog.cpp
    CustomLineEdit.h
    CustomLineEdit.cpp
    CustomTooltip.h
)

setup_ui(${PROJECT_NAME} ui_files)

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
