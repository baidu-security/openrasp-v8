cmake_minimum_required(VERSION 3.0.0)

project(go CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(..)

if (NOT TARGET base)
    add_subdirectory(../base base)
endif() 

add_library(openrasp_v8_go)
target_link_libraries(openrasp_v8_go base)

add_custom_command(TARGET openrasp_v8_go POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -DOPENRASP_V8_PATH=${CMAKE_CURRENT_BINARY_DIR}
  -DV8_INC_DIR="$<TARGET_PROPERTY:base,V8_INC_DIR>"
  -DV8_LINK_LIBS="$<TARGET_PROPERTY:base,V8_LINK_LIBS>"
  -P ${PROJECT_SOURCE_DIR}/PkgConfig.cmake)