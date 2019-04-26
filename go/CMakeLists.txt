cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake/cxx_flag_overrides.cmake)

project(go LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

if (NOT TARGET base)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../base base)
endif() 

add_library(openrasp_v8_go)
target_link_libraries(openrasp_v8_go base)

add_custom_command(TARGET openrasp_v8_go POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -DOPENRASP_V8_PATH=${CMAKE_CURRENT_BINARY_DIR}
  -DINCLUDE_DIRECTORIES="$<TARGET_PROPERTY:base,INTERFACE_INCLUDE_DIRECTORIES>"
  -DLINK_LIBRARIES="$<TARGET_PROPERTY:base,INTERFACE_LINK_LIBRARIES>"
  -DOUTPUT_NAME="$<TARGET_FILE:openrasp_v8_go>"
  -P ${CMAKE_CURRENT_SOURCE_DIR}/PkgConfig.cmake)
