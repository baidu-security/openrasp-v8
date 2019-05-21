cmake_minimum_required(VERSION 3.0.0)

project(prebuilts LANGUAGES CXX)

string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)

set(PREBUILTS_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEM_NAME})

add_library(prebuilts INTERFACE)
target_include_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/include)
include(${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake/detect_architecture.cmake)

if(ARCH64)
  target_link_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/lib64)
 else()
  target_link_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/lib)
endif()

target_link_libraries(prebuilts INTERFACE v8_monolith)

if(UNIX)
    target_link_libraries(prebuilts INTERFACE pthread)
endif()

if(WIN32)
    target_link_libraries(prebuilts INTERFACE winmm dbghelp shlwapi)
endif()

if(UNIX AND NOT APPLE)
    target_include_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/include/c++/v1)
    target_compile_options(prebuilts INTERFACE -nostdinc++)
    target_link_libraries(prebuilts INTERFACE -nodefaultlibs c++ c++abi m c rt gcc_s)
endif()