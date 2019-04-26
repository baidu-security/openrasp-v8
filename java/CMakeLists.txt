cmake_minimum_required(VERSION 3.0.0)

project(java CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${PROJECT_SOURCE_DIR}/..)

if (NOT TARGET base)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../base base)
endif() 

include(FindJNI)
include_directories(${JNI_INCLUDE_DIRS})

add_library(openrasp_v8_java SHARED com_baidu_openrasp_plugin_v8_V8.cc utils.cc context.cc)
target_link_libraries(openrasp_v8_java base)
