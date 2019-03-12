cmake_minimum_required(VERSION 3.0.0)

project(php CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(..)

add_subdirectory(../base base)
include_directories(${V8_PATH}/include)

add_library(openrasp_v8_php STATIC $<TARGET_OBJECTS:base> utils.cc)
