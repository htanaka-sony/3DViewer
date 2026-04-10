cmake_minimum_required(VERSION 3.19)

# ビルドタイプの設定
# 現時点ではWithPDBのみ（ReleaseでもPDB生成用）
#set(APP_BUILD_TYPE "WithPDB")

# Version指定
project(3DViewer VERSION 1.0.0)

# std::execution::par使っているのでMSVCに限定する（MinGW対応していない）
if(NOT MSVC)
    message(FATAL_ERROR "This project can only be built with Visual Studio (MSVC).")
endif()

add_definitions(
  -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

string(TIMESTAMP BUILD_DATE "%Y%m%d")
add_compile_definitions(BUILD_DATE="${BUILD_DATE}")

# 環境で書き換える
set(MY_QT_SOURCE_DIR "C:/Qt/6.5.3/Src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # exeやDLLの出力先
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # DLLの出力先
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # 静的ライブラリの出力先

### 3rdParty関連
set(EARCUT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/earcut/earcut-v2.2.4/include/mapbox)
set(TINYCOLORMAP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/tinycolormap/tinycolormap-20250606/include)
set(LZ4_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/lz4/lz4-v1.10.0/lib)

# ライセンスファイルのコピー元とコピー先を設定
set(LICENSE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/tinycolormap/tinycolormap-20250606/LICENSE|tinycolormap/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/earcut/earcut-v2.2.4/LICENSE|earcut/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/lz4/lz4-v1.10.0/lib/LICENSE|lz4/LICENSE"
    "${MY_QT_SOURCE_DIR}/LICENSES/LGPL-3.0-only.txt|Qt/LICENSE.LGPL3"
)
set(LICENSE_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/LICENSE)

# LICENSEフォルダを作成
add_custom_target(CopyLicenses ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LICENSE_OUTPUT_DIR}
    COMMENT "Creating LICENSE directory at ${LICENSE_OUTPUT_DIR}"
)

# ライセンスファイルをコピー
foreach(LICENSE_PAIR ${LICENSE_FILES})
    string(REPLACE "|" ";" LICENSE_PAIR_LIST ${LICENSE_PAIR})
    list(GET LICENSE_PAIR_LIST 0 LICENSE_SRC)
    list(GET LICENSE_PAIR_LIST 1 LICENSE_DEST)

    if(EXISTS ${LICENSE_SRC})
        add_custom_command(
            TARGET CopyLicenses
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LICENSE_SRC} ${LICENSE_OUTPUT_DIR}/${LICENSE_DEST}
            COMMENT "Copying ${LICENSE_SRC} to ${LICENSE_OUTPUT_DIR}/${LICENSE_DEST}"
            VERBATIM
        )
    else()
        message(WARNING "LICENSE file not found: ${LICENSE_SRC}")
    endif()
endforeach()

# モジュールパス
set(OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_CXX_STANDARD 20)

include(CMakeUtils/UtilityFunction.cmake)
include(CMakeUtils/QtUtil.cmake)
include(CMakeUtils/3DViewerUtil.cmake)

add_subdirectory(Core)
add_subdirectory(Render)
#add_subdirectory(MainGUI)
add_subdirectory(VoxViewer)
add_subdirectory(Main)
