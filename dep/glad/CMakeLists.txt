cmake_minimum_required(VERSION 3.16)
project(glad)


file(GLOB_RECURSE headers_list "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources_list "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_library(glad STATIC ${sources_list} ${headers_list})
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
