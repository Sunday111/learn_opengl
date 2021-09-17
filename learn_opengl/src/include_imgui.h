#pragma once

// clang-format off
#ifdef __clang__
    #define include_imgui_begin \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
        static_assert(true, "")
#elif defined(__GNUC__) || defined(__GNUG__)
    #define include_imgui_begin \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wduplicated-branches\"") \
        _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"") \
        static_assert(true, "")
#elif defined(_MSC_VER)
    #define include_imgui_begin \
        warning_push \
        _Pragma("warning(disable : 4201)") \
        static_assert(true, "")
#endif

#define include_imgui_end warning_pop static_assert(true, "")
// clang-format on

include_imgui_begin;
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/imgui.h>
include_imgui_end;