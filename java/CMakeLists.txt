cmake_minimum_required(VERSION 3.12.0)

project(java LANGUAGES CXX)

find_package(JNI)

add_library(openrasp_v8_java SHARED com_baidu_openrasp_v8_V8.cc com_baidu_openrasp_v8_Context.cc utils.cc)
target_include_directories(openrasp_v8_java PUBLIC ${JNI_INCLUDE_DIRS})
add_dependencies(openrasp_v8_java base prebuilts)
target_link_libraries(openrasp_v8_java PUBLIC base prebuilts)