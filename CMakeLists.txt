cmake_minimum_required(VERSION 3.20)

include(ExternalProject)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project (learn_opengl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(GIT_EXECUTABLE git)
find_package(OpenGL REQUIRED)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/fmt)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/spdlog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/stb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/imgui)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/json)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/learn_opengl)


## prints all cmake variables
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message("${_variableName}=${${_variableName}}")
#endforeach()