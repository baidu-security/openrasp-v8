cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake/cxx_flag_overrides.cmake)

project(java LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

if (NOT TARGET base)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../base base)
endif() 

include(FindJNI)
include_directories(${JNI_INCLUDE_DIRS})

add_library(openrasp_v8_java SHARED com_baidu_openrasp_plugin_v8_V8.cc utils.cc context.cc)
target_link_libraries(openrasp_v8_java base)
