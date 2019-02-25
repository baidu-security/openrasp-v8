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

aux_source_directory(. DIR_SRCS)

add_library(base ${DIR_SRCS})