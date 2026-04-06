cmake_minimum_required(VERSION 3.19)

set(MY_UTILS__QT_UTIL_LOADED TRUE CACHE INTERNAL "QtUtil.cmake loaded indicator")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(STATUS "Qt version: ${QT_VERSION}")


# param1: target
# keyword param1: packages(List): Qt Package Names
# keyword param2: require_private_header(Bool): QtのPrivateヘッダをIncludeするかどうか
function(linkQt target)
    message(STATUS "Link Qt for ${target}")

    set(options require_private_header)
    set(oneValueArgs) # None
    set(multiValueArgs packages)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Qt5の場合、SvgWidgets->Svgに変更する
    set(required_packages "")
    if (QT_VERSION_MAJOR LESS_EQUAL 5)
        foreach(pkg IN LISTS arg_packages)
            if("${pkg}" STREQUAL "SvgWidgets")
                list(APPEND required_packages "Svg")
            else()
                list(APPEND required_packages "${pkg}")
            endif()
        endforeach()
    else()
        set(required_packages ${arg_packages})
    endif()

    # 基本package設定
    message(DEBUG "[${target}] packages: ${required_packages}")
    find_package(QT NAMES Qt6 Qt5 COMPONENTS ${required_packages} REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${required_packages} REQUIRED)

    # Link
    foreach(pkg IN LISTS required_packages)
        target_link_libraries(${target} PRIVATE
            Qt${QT_VERSION_MAJOR}::${pkg}
        )
    endforeach()

    # Private headerのInclude
    message(DEBUG "[${target}] require_private_header: ${arg_require_private_header}")
    if(${arg_require_private_header})
        foreach(pkg IN LISTS required_packages)
            target_include_directories(${target} PRIVATE
                ${Qt${QT_VERSION_MAJOR}${pkg}_PRIVATE_INCLUDE_DIRS}
            )
        endforeach()
    endif()
endfunction()

# brief 翻訳ファイル設定を行う.
# param ts_files: .tsファイルのList
macro(addTranslations)
    set(options) # None
    set(oneValueArgs) # None
    set(multiValueArgs ts_files)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # このfind_packageはadd_libraryなどの前に行う必要がある?
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core LinguistTools REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core LinguistTools REQUIRED)

    # deprecated, Qt6.2からはqt_add_translationsを用いる
    qt_add_translation(qm_files ${arg_ts_files})

    # Copy resource collection file for translation files to CMAKE_CURRENT_BINARY_DIR
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/translations.qrc
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/translations.qrc
                ${PROJECT_BINARY_DIR}/translations.qrc
        DEPENDS ${qm_files}
        )
endmacro()


function(copyQtFiles target output_dir)
    message(STATUS "copyQtFiles")

    set(options with_pdb)
    set(oneValueArgs) # None
    set(multiValueArgs packages)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    message(DEBUG "with_pdb: ${arg_with_pdb}")
    message(DEBUG "packages: ${arg_packages}")

    if (NOT DEFINED QT_VERSION_MAJOR)
        find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
    endif()
    message(STATUS "QT Version: ${QT_VERSION_MAJOR}")
    set(MY_QT_BIN_DIR ${QT_DIR}/../../../bin)

    foreach(file IN LISTS arg_packages)
        string(PREPEND file "Qt${QT_VERSION_MAJOR}")
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(APPEND file "d")
        endif()

        list(APPEND qt_files ${file})
    endforeach()

    # DLLコピー.
    foreach(file IN LISTS qt_files)
        add_custom_command(TARGET ${target}
            POST_BUILD
            COMMAND  echo "[POST Build Process ${target}] Copy ${file}.dll"
            COMMAND ${CMAKE_COMMAND} -E copy ${MY_QT_BIN_DIR}/${file}.dll ${output_dir}
        )
    endforeach()

    # PDBファイル(デバッグシンボル)のコピー.
    if (${arg_with_pdb} OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
        foreach(file IN LISTS qt_files)
            add_custom_command(TARGET ${target}
                POST_BUILD
                COMMAND  echo "[POST Build Process ${target}] Copy ${file}.pdb"
                COMMAND ${CMAKE_COMMAND} -E copy ${MY_QT_BIN_DIR}/${file}.pdb ${output_dir}
            )
        endforeach()
    endif()
endfunction()

# windeployqt.exeを実行する
# 関数名: runWindeployqt
# 引数1 : ターゲット名
# 引数2 : コピー先フォルダ
# 引数2 : Deployするexeファイル
# 効果  : windeployqt.exeを実行する
function(runWindeployqt target output_dir target_file)

    # WINDEPLOYQT_EXECUTABLE変数はQtによって生成される。

    if(NOT DEFINED ${WINDEPLOYQT_EXECUTABLE})
        set(MY_QT_BIN_DIR ${QT_DIR}/../../../bin)
        set(WINDEPLOYQT_PATH ${MY_QT_BIN_DIR}/windeployqt.exe)
        get_filename_component(WINDEPLOYQT_EXECUTABLE "${WINDEPLOYQT_PATH}" ABSOLUTE)
    endif()
    if(NOT EXISTS ${WINDEPLOYQT_EXECUTABLE})
        message(FATAL_ERROR "[FATAL] NOT found windeployqt.exe")
    endif()

    message(STATUS "[INFO] found windeployqt at ${WINDEPLOYQT_EXECUTABLE}")

    add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND ${WINDEPLOYQT_EXECUTABLE} ${output_dir}/${target_file}
    )
endfunction()

function(copyQtPDB target output_dir)

    set(options)
    set(oneValueArgs) # None
    set(multiValueArgs packages)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Qtのbinディレクトリを設定
    set(MY_QT_BIN_DIR ${QT_DIR}/../../../bin)

    # QtのPDBファイルリストを作成
    set(qt_files)  # 初期化
    foreach(file IN LISTS arg_packages)
        string(PREPEND file "Qt${QT_VERSION_MAJOR}")  # Qtのバージョンを付加
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(APPEND file "d")  # デバッグビルドの場合は"d"を付加
        endif()
        list(APPEND qt_files ${file})  # PDBファイル名をリストに追加

        message(STATUS qt_files)
    endforeach()

    # PDBファイルをコピーするコマンドを追加
    foreach(file IN LISTS qt_files)
        add_custom_command(TARGET ${target}
            POST_BUILD
            COMMAND echo "[POST Build Process ${target}] Copy ${file}.pdb"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MY_QT_BIN_DIR}/${file}.pdb
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        )
    endforeach()
endfunction()
