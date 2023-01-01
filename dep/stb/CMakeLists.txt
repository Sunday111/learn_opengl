cmake_minimum_required(VERSION 3.16)

set(target_name stb)

add_library(${target_name} INTERFACE)
target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${target_name} INTERFACE -DSTB_IMAGE_IMPLEMENTATION)
