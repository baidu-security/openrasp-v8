cmake_minimum_required(VERSION 3.0.0)

project(prebuilts LANGUAGES CXX)

string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)

set(PREBUILTS_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEM_NAME})
set(PREBUILTS_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEM_NAME} PARENT_SCOPE)

if(NOT IS_DIRECTORY ${PREBUILTS_PREFIX})
    execute_process(COMMAND bash "-c" "${CMAKE_CURRENT_SOURCE_DIR}/fetch_prebuilts.sh" RESULT_VARIABLE ret)
    if(NOT ret EQUAL "0")
        message(FATAL_ERROR "Failed to to fetch prebuilts")
    endif()
endif()

add_library(prebuilts INTERFACE)
target_include_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/include)
include(${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cmake/detect_architecture.cmake)

if(ARCH64)
    set(LIBS_PREFIX ${PREBUILTS_PREFIX}/lib64)
else()
    set(LIBS_PREFIX ${PREBUILTS_PREFIX}/lib32)
endif()

file(GLOB LIBS LIST_DIRECTORIES false ${LIBS_PREFIX}/*)

if(APPLE)
    find_package(CURL)
    find_package(ZLIB)
    target_link_libraries(prebuilts INTERFACE CURL::libcurl ZLIB::ZLIB)
    target_link_libraries(prebuilts INTERFACE ${LIBS})
endif()

if(WIN32)
    target_compile_definitions(prebuilts INTERFACE CURL_STATICLIB)
    target_link_libraries(prebuilts INTERFACE ${LIBS})
    target_link_libraries(prebuilts INTERFACE winmm.lib dbghelp.lib shlwapi.lib ws2_32.lib crypt32.lib)
endif()

if(UNIX AND NOT APPLE)
    target_include_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/include/c++/v1)
    target_compile_options(prebuilts INTERFACE -nostdinc++)
    list(TRANSFORM LIBS PREPEND "-Wl,")
    target_link_libraries(prebuilts INTERFACE -Wl,--start-group ${LIBS} -Wl,--end-group)
    target_link_libraries(prebuilts INTERFACE -nodefaultlibs -lm -lc -lrt -lgcc_s -ldl -lpthread)
endif()
