cmake_minimum_required(VERSION 3.0.0)

project(openrasp-v8 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(base)
add_subdirectory(go)
add_subdirectory(java)
add_subdirectory(php)