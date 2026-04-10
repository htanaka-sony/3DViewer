cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

project(3DViewer)

if(WIN32)
    set(GUI_TYPE WIN32)
endif(WIN32)

if(EMV_FORCE_CONSOLE)
    unset(GUI_TYPE)
endif()


set(APP_ICON_RESOURCE_WINDOWS icon.rc)

add_executable(${PROJECT_NAME} ${GUI_TYPE}
    main.cpp
    ${APP_ICON_RESOURCE_WINDOWS}
    resources.qrc
    )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${PROJECT_SOURCE_DIR}/images/3DViewer.ico
            ${CMAKE_BINARY_DIR}/bin/icons/3DViewer.ico
    )

target_include_directories(${PROJECT_NAME} PRIVATE
    ../VoxViewer
    ../VoxViewer/ui_files
    )

target_link_libraries(${PROJECT_NAME} PRIVATE VoxViewer)

# 使用するQtのコンポーネントをリストとして宣言
set(QT_COMPONENTS Core Gui Widgets)



# Qtライブラリの設定、QtDLLのコピー、PDB生成
setupQtAndPDBMacro(${PROJECT_NAME} exe "${QT_COMPONENTS}")
