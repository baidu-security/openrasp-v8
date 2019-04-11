cmake_minimum_required(VERSION 3.0.0)

project(php CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${PROJECT_SOURCE_DIR}/..)

if (NOT TARGET base)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../base base)
endif() 

add_library(openrasp_v8_php STATIC utils.cc)
target_link_libraries(openrasp_v8_php base)
