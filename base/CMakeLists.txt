cmake_minimum_required(VERSION 3.0.0)

project(base CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_path(V8_PATH include/v8.h)
include_directories(${V8_PATH}/include)

find_program(XXD xxd)
message("-- Execute xxd to covert js code to c header file")
if(XXD)
    execute_process(COMMAND sh -c "${XXD} -i console.js > js.h" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/js ERROR_VARIABLE XXDERR)
    execute_process(COMMAND sh -c "${XXD} -i checkpoint.js >> js.h"  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/js ERROR_VARIABLE XXDERR)
    execute_process(COMMAND sh -c "${XXD} -i context.js >> js.h" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/js ERROR_VARIABLE XXDERR)
    execute_process(COMMAND sh -c "${XXD} -i error.js >> js.h" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/js ERROR_VARIABLE XXDERR)
    execute_process(COMMAND sh -c "${XXD} -i rasp.js >> js.h" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/js ERROR_VARIABLE XXDERR)
    execute_process(COMMAND sh -c "${XXD} -i sql_tokenize.js >> js.h" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/js ERROR_VARIABLE XXDERR)
    execute_process(COMMAND sh -c "${XXD} -i cmd_tokenize.js >> js.h" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/js ERROR_VARIABLE XXDERR)
    if(XXDERR)
        message("${XXDERR}")
    endif()
endif()

aux_source_directory(. BASE_SRCS)

add_library(base OBJECT ${BASE_SRCS})

# testing #
option(BUILD_TESTING "Build the testing tree." OFF)
option(BUILD_COVERAGE "Enable coverage reporting" OFF)
if((BUILD_TESTING OR BUILD_COVERAGE) AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR) AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    find_package(Catch2 REQUIRED PATHS "${PROJECT_SOURCE_DIR}/../vendors/Catch2")
    aux_source_directory(tests BASE_TESTS_SRCS)
    add_executable(tests ${BASE_TESTS_SRCS})
    target_link_libraries(tests base ${V8_PATH}/lib/libv8_monolith.a Catch2::Catch2)

    include(CTest)
    include(Catch)
    catch_discover_tests(tests)

    if(BUILD_COVERAGE)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../vendors/cmake")
        include(CodeCoverage)
        append_coverage_compiler_flags()
        set(COVERAGE_LCOV_EXCLUDES '*usr*' '*libv8*' '*tests*' '*catch2*')
        setup_target_for_coverage_lcov(NAME coverage EXECUTABLE tests DEPENDENCIES tests)
    endif()
endif()
