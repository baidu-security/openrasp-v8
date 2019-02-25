cmake_minimum_required(VERSION 3.0.0)

project(java CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(..)

find_package(JNI REQUIRED)
include_directories(${JAVA_INCLUDE_PATH})
include_directories(${JAVA_INCLUDE_PATH2})

find_path(V8_PATH include/v8.h)
include_directories(${V8_PATH}/include)

add_subdirectory(../base base)

add_library(openrasp_v8_java SHARED com_baidu_openrasp_plugin_v8_V8.cc utils.cc context.cc)

target_link_libraries(openrasp_v8_java base ${V8_PATH}/lib/libv8_monolith.a)  
find_library(LIBRT rt)
if(LIBRT)
    target_link_libraries(openrasp_v8_java ${LIBRT})
endif()