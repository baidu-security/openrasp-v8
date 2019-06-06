cmake_minimum_required(VERSION 3.0.0)

project(prebuilts LANGUAGES CXX)

string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)

set(PREBUILTS_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEM_NAME})

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
    target_link_libraries(prebuilts INTERFACE ${LIBS})
    target_link_libraries(prebuilts INTERFACE -lcurl)
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
