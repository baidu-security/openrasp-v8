cmake_minimum_required(VERSION 3.0.0)
set(PREFIX ${CMAKE_CURRENT_LIST_DIR})
list(JOIN V8_LINK_LIBS " " V8_LINK_LIBS)
configure_file(${PREFIX}/openrasp-v8.pc.in ${PREFIX}/openrasp-v8.pc)
configure_file(${PREFIX}/v8.pc.in ${PREFIX}/v8.pc)