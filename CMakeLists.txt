cmake_minimum_required(VERSION 3.21)

# =============================================================
#  VAWSpouseSwitcher — SKSE plugin (Skyrim SE 1.5.97)
#
#  Uses CommonLibSSE-NG (Color-Glass Studios vcpkg port). The helper
#  function add_commonlibsse_plugin() comes from that port and
#  auto-generates the SKSEPluginInfo() declaration TU, which exports
#  the SKSEPlugin_Version symbol SKSE needs to accept the DLL.
# =============================================================

project(
    VAWSpouseSwitcher
    VERSION 1.0.0
    DESCRIPTION "Runtime association-type (Spouse) switcher for Vittoria's Alternate Wedding KR"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(sources
    src/main.cpp)

# --- CommonLibSSE-NG ---
find_package(CommonLibSSE CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${sources})

# --- Precompiled header (REQUIRED FIX) ---
# add_commonlibsse_plugin() generates __VAWSpouseSwitcherPlugin.cpp whose
# first line is '#include "REL/Relocation.h"'. Relocation.h is NOT
# self-sufficient: it relies on the namespace aliases (REL::stl,
# REL::WinAPI, std::literals) and the std-library includes that
# "SKSE/Impl/PCH.h" sets up before it includes Relocation.h at its very
# bottom (RE/Skyrim.h includes SKSE/Impl/PCH.h first for the same reason).
# Precompiling that header force-includes (/FI) it into EVERY TU -
# including the auto-generated one - which fixes:
#   error C2653: 'stl': is not a class or namespace name
# The double-bracket form [[...]] makes CMake resolve the header through
# the include directories instead of the source tree.
target_precompile_headers(${PROJECT_NAME} PRIVATE
    [["SKSE/Impl/PCH.h"]])

# --- MSVC-only flags ---
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /permissive-
        /Zc:preprocessor
        /Zc:__cplusplus
        /EHsc
        /utf-8
        /wd4100
        /wd4244
    >)

# --- Output: a plain VAWSpouseSwitcher.dll at the build root ---
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "VAWSpouseSwitcher"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}")

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
