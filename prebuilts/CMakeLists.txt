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
    target_link_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/lib32)
endif()

if(APPLE)
    target_link_libraries(prebuilts INTERFACE libv8_monolith.a libcpr.a)
    target_link_libraries(prebuilts INTERFACE curl)
endif()

if(WIN32)
    target_compile_definitions(prebuilts INTERFACE CURL_STATICLIB)
    target_link_libraries(prebuilts INTERFACE v8_monolith.lib cpr.lib curl.lib z.lib)
    target_link_libraries(prebuilts INTERFACE winmm dbghelp shlwapi ws2_32 crypt32)
endif()

if(UNIX AND NOT APPLE)
    target_include_directories(prebuilts INTERFACE ${PREBUILTS_PREFIX}/include/c++/v1)
    target_compile_options(prebuilts INTERFACE -nostdinc++)
    target_link_libraries(prebuilts INTERFACE -Wl,-Bstatic -lv8_monolith -lcpr -lcurl -lssl -lcrypto -lz -lc++ -lc++abi -Wl,-Bdynamic)
    target_link_libraries(prebuilts INTERFACE -nodefaultlibs -lm -lc -lrt -lgcc_s -ldl -lpthread)
endif()
