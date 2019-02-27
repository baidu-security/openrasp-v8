cmake_minimum_required(VERSION 3.0.0)

project(php CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(..)

find_path(V8_PATH include/v8.h)
include_directories(${V8_PATH}/include)

add_subdirectory(../base base)

add_library(openrasp_v8_php STATIC utils.cc)

target_link_libraries(openrasp_v8_php base)