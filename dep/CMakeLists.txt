cmake_minimum_required(VERSION 3.16)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/spdlog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/stb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/imgui)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/json)