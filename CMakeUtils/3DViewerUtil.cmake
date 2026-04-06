cmake_minimum_required(VERSION 3.19)

set(MY_UTILS__3DVIEWER_UTIL_LOADED TRUE CACHE INTERNAL "3DViewer.cmake loaded indicator")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(STATUS "Qt version: ${QT_VERSION}")

macro(setupQtAndPDBMacro target target_ext qt_component)
    # Qtライブラリのリンク
    linkQt(${target} packages ${qt_component} require_private_header)

    # PDBファイルの生成とコピー (APP_BUILD_TYPEがWithPDBまたはCMAKE_BUILD_TYPEがDebugの場合)
    if (APP_BUILD_TYPE STREQUAL "WithPDB" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
        # 自身のPDBファイル
        if(MSVC)
            target_compile_options(${target} PRIVATE "/Zi")  # デバッグ情報を生成
            target_link_options(${target} PRIVATE "/DEBUG")  # PDBファイルを生成
        endif()

        # Qt関連のPDBファイルのコピー
        copyQtPDB(${target} ${OUTPUT_DIR} packages ${qt_component})
    endif()

    # windeployqtの実行 (Windowsのみ)
    if(WIN32)
        set(TARGET_FILE ${target}.${target_ext})
        runWindeployqt(${target} ${OUTPUT_DIR} ${TARGET_FILE} packages ${qt_component})
    endif()
endmacro()


