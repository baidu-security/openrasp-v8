cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake/cxx_flag_overrides.cmake)

project(base LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} BASE_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/flex FLEX_SRCS)

add_library(base INTERFACE)
target_sources(base INTERFACE ${BASE_SRCS} ${FLEX_SRCS})
target_include_directories(base INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(../prebuilts prebuilts)
target_link_libraries(base INTERFACE prebuilts)

set(JS_SRCS console.js checkpoint.js context.js flex.js rasp.js)
list(TRANSFORM JS_SRCS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/js/)
find_program(XXD xxd ${PREBUILTS_PREFIX}/bin)
add_custom_target(builtinjs2cheader
    COMMAND rm -rf gen && mkdir gen
    COMMAND cat ${JS_SRCS} > gen/builtins
    COMMAND ${XXD} -i gen/builtins > gen/builtins.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(base builtinjs2cheader)

# testing #
option(BUILD_TESTING "Build the testing tree." OFF)
option(BUILD_COVERAGE "Enable coverage reporting" OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(BUILD_TESTING ON)
    find_package(Catch2 REQUIRED PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../vendors/Catch2")
    aux_source_directory(tests BASE_TESTS_SRCS)
    add_executable(tests ${BASE_TESTS_SRCS})
    target_link_libraries(tests base Catch2::Catch2)

    include(CTest)
    include(Catch)
    catch_discover_tests(tests)

    if(BUILD_COVERAGE)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake")
        include(CodeCoverage)
        append_coverage_compiler_flags()
        set(COVERAGE_LCOV_EXCLUDES '*usr*' '*libv8*' '*tests*' '*catch2*' 'v1/*' '*flex*')
        setup_target_for_coverage_lcov(NAME coverage EXECUTABLE tests DEPENDENCIES tests)
        if(LIBCXX_PATH)
            target_link_libraries(tests gcov)
        endif()
    endif()
endif()
