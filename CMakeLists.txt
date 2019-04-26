cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/vendors/cmake/cxx_flag_overrides.cmake)

project(openrasp-v8 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(base)
add_subdirectory(go)
add_subdirectory(java)
add_subdirectory(php)