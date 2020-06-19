cmake_minimum_required(VERSION 3.12.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/vendors/cmake/cxx_flag_overrides.cmake)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(openrasp-v8 LANGUAGES CXX)

option(ENABLE_IAST "enable IAST" OFF)

enable_testing()

if (ENABLE_LANGUAGES STREQUAL "all")
  set(LANGS php java go)
else()
  set(LANGS ${ENABLE_LANGUAGES})
endif()

add_subdirectory(prebuilts)
add_subdirectory(base)
foreach(LANG IN LISTS LANGS)
  add_subdirectory(${LANG})
endforeach()