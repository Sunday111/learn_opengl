cmake_minimum_required(VERSION 3.16)

project(learn_opengl)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/dep/sunday_cmake_presets")

include(CheckIPOSupported)
include(enable_lto)

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif(POLICY CMP0048)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)

check_ipo_supported(RESULT ipo_supported)

enable_lto_globally()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/learn_opengl)
