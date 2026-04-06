cmake_minimum_required(VERSION 3.19)

set(MY_UTILS__UTILITY_FUNCTION_LOADED TRUE CACHE INTERNAL "UtilityFunction.cmake loaded indicator")

function(setDefaultOutputDir target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
endfunction()

function(setLibraryOutputDir target libname)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${libname}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${libname}
    )
endfunction()

# collectSrcFilesを実行したあとに呼ぶ。
macro(excludeFromSrc DIR_NAME)
    list(FILTER SOURCES   EXCLUDE REGEX "^${DIR_NAME}/.*")
    list(FILTER HEADERS   EXCLUDE REGEX "^${DIR_NAME}/.*")
    list(FILTER FORMS     EXCLUDE REGEX "^${DIR_NAME}/.*")
    list(FILTER RESOURCES EXCLUDE REGEX "^${DIR_NAME}/.*")
    list(FILTER TS_FILES  EXCLUDE REGEX "^${DIR_NAME}/.*")
    list(FILTER DOCS      EXCLUDE REGEX "^${DIR_NAME}/.*")
endmacro()


# @brief: macro呼び出し元のCMakeLists.txt直下のソールファイルパスを取得する.
# param exclude_dirs: 取得から除外するフォルダ名
macro(collectSrcFiles)
    set(options) # None
    set(oneValueArgs) # None
    set(multiValueArgs exclude_dirs)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 直下のソースファイル群を取得する
    file(GLOB_RECURSE SOURCES     RELATIVE ${PROJECT_SOURCE_DIR} *.cpp)
    file(GLOB_RECURSE SOURCES_CXX RELATIVE ${PROJECT_SOURCE_DIR} *.cxx)
    file(GLOB_RECURSE SOURCES_CU  RELATIVE ${PROJECT_SOURCE_DIR} *.cu)
    file(GLOB_RECURSE HEADERS     RELATIVE ${PROJECT_SOURCE_DIR} *.h)
    file(GLOB_RECURSE HEADERS_HPP RELATIVE ${PROJECT_SOURCE_DIR} *.hpp)
    file(GLOB_RECURSE HEADERS_HXX RELATIVE ${PROJECT_SOURCE_DIR} *.hxx)
    file(GLOB_RECURSE HEADERS_CUH RELATIVE ${PROJECT_SOURCE_DIR} *.cuh)
    file(GLOB_RECURSE FORMS       RELATIVE ${PROJECT_SOURCE_DIR} *.ui)
    file(GLOB_RECURSE RESOURCES   RELATIVE ${PROJECT_SOURCE_DIR} *.qrc)
    file(GLOB_RECURSE TS_FILES    RELATIVE ${PROJECT_SOURCE_DIR} *.ts)
    file(GLOB_RECURSE DOCS        RELATIVE ${PROJECT_SOURCE_DIR} *.md)

    # .cxxと.hxxをSOURCESとHEADERSに統合
    list(APPEND SOURCES ${SOURCES_CXX})
    list(APPEND SOURCES ${SOURCES_CU})
    list(APPEND HEADERS ${HEADERS_HPP})
    list(APPEND HEADERS ${HEADERS_HXX})
    list(APPEND HEADERS ${HEADERS_CUH})

    # 変数を破棄
    unset(SOURCES_CXX)
    unset(SOURCES_CU)
    unset(HEADERS_HPP)
    unset(HEADERS_HXX)
    unset(HEADERS_CUH)

    # deprecatedフォルダ以下のソースは除外する
    excludeFromSrc(deprecated)

    # その他除外フォルダ
    foreach(exclude_dir IN LISTS arg_exclude_dirs)
        excludeFromSrc(${exclude_dir})
    endforeach()
endmacro()


function(printAllCMakeVar)
    message(STATUS "========================================================")
    message(STATUS "================ Begin printAllCMakeVar ================")
    message(STATUS "========================================================")

    get_cmake_property(all_variables VARIABLES)
    list(APPEND CMAKE_MESSAGE_INDENT "    ")
    foreach(variable ${all_variables})
        message(STATUS "${variable} = ${${variable}}")
    endforeach()
    list(POP_BACK CMAKE_MESSAGE_INDENT)

    message(STATUS "========================================================")
    message(STATUS "====================== Cache ===========================")
    message(STATUS "========================================================")

    get_cmake_property(all_cache_variables CACHE_VARIABLES)
    list(APPEND CMAKE_MESSAGE_INDENT "    ")
    foreach(variable ${all_cache_variables})
        message(STATUS "${variable} = ${${variable}}")
    endforeach()
    list(POP_BACK CMAKE_MESSAGE_INDENT)

    message(STATUS "========================================================")
    message(STATUS "================ End printAllCMakeVar ==================")
    message(STATUS "========================================================")
endfunction()

# @briief: リストを開業して表示させる
# @arg1 : リスト名
# @arg2 : リスト
# @arg3 : リストアイテムのPrefixのインデント数
function(print_list readable_list_name readable_list indent_spaces)
    # リスト名を出力
    message(STATUS "${readable_list_name}:")
    # 数値からインデントの文字列を生成
    string(REPEAT " " ${indent_spaces} indent)
    # リストをループして出力
    foreach(item IN LISTS ${readable_list})
        message(STATUS "${indent}${item}")
    endforeach()
endfunction()
