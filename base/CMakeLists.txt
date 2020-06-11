cmake_minimum_required(VERSION 3.12.0)

project(base LANGUAGES CXX)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} BASE_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/flex FLEX_SRCS)

add_library(base OBJECT ${BASE_SRCS} ${FLEX_SRCS})
target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_dependencies(base prebuilts)
target_link_libraries(base PUBLIC prebuilts)

set(JS_SRCS console.js flex.js rasp.js)
list(TRANSFORM JS_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/js/)
# 使用xxd把文件内容转换为c字符数组
find_program(XXD xxd ${PREBUILTS_PREFIX}/bin)
add_custom_target(builtinjs2cheader
    COMMAND rm -rf gen && mkdir gen
    COMMAND cat ${JS_SRCS} > gen/builtins
    COMMAND ${XXD} -i gen/builtins > gen/builtins.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(base PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(base builtinjs2cheader)

# 添加cpr源码一起编译
target_include_directories(base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cpr/include)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cpr/cpr CPR_SRCS)
target_sources(base PRIVATE ${CPR_SRCS})

# testing #
option(BUILD_TESTING "Build the testing tree." OFF)
option(BUILD_COVERAGE "Enable coverage reporting" OFF)
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug" OR BUILD_TESTING)
    set(BUILD_TESTING ON)
    find_package(Catch2 REQUIRED PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../vendors/Catch2")
    aux_source_directory(tests BASE_TESTS_SRCS)
    add_executable(tests ${BASE_TESTS_SRCS})
    target_include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cpr/include)
    target_link_libraries(tests base Catch2::Catch2)

    include(CTest)
    include(Catch)
    catch_discover_tests(tests)

    if(BUILD_COVERAGE)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake")
        include(CodeCoverage)
        append_coverage_compiler_flags()
        set(COVERAGE_LCOV_EXCLUDES '*tests*' '*flex*' '*lex.yy.c*' '*usr*' '*vendors*' '*prebuilts*')
        setup_target_for_coverage_lcov(NAME coverage EXECUTABLE tests DEPENDENCIES tests)
    endif()
endif()
