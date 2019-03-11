cmake_minimum_required(VERSION 3.0.0)

project(go CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(..)

find_path(V8_PATH include/v8.h)
include_directories(${V8_PATH}/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/../base base)

add_library(openrasp_v8_go STATIC $<TARGET_OBJECTS:base>)

add_custom_command(TARGET openrasp_v8_go POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -DOPENRASP_V8_PATH=${CMAKE_CURRENT_BINARY_DIR}
  -DV8_PATH=${V8_PATH}
  -P ${PROJECT_SOURCE_DIR}/PkgConfig.cmake)