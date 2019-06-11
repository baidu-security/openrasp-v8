cmake_minimum_required(VERSION 3.12.0)

project(go LANGUAGES CXX)

add_library(openrasp_v8_go)
add_dependencies(openrasp_v8_go base prebuilts)
target_link_libraries(openrasp_v8_go PUBLIC base prebuilts)

add_custom_command(TARGET openrasp_v8_go POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -DOPENRASP_V8_PATH=${CMAKE_CURRENT_BINARY_DIR}
  -DINCLUDE_DIRECTORIES="$<TARGET_PROPERTY:openrasp_v8_go,INTERFACE_INCLUDE_DIRECTORIES>"
  -DLINK_LIBRARIES="$<TARGET_PROPERTY:prebuilts,INTERFACE_LINK_LIBRARIES>"
  -DTARGET_FILE="$<TARGET_FILE:openrasp_v8_go>"
  -P ${CMAKE_CURRENT_SOURCE_DIR}/PkgConfig.cmake)
