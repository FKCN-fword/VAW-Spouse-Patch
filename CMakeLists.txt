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
# add_commonlibsse_plugin() auto-generates __VAWSpouseSwitcherPlugin.cpp,
# which has two problems:
#
# 1. Its first line is '#include "REL/Relocation.h"', but Relocation.h is NOT
#    self-sufficient: it relies on the aliases (REL::stl, REL::WinAPI) and
#    Windows declarations that "SKSE/Impl/PCH.h" sets up first (RE/Skyrim.h
#    includes PCH.h at its line 3 for exactly this reason).
#    -> error C2653: 'stl': is not a class or namespace name
#
# 2. It calls SKSEPluginInfo(...) with arguments like
#        .Name = "VAWSpouseSwitcher"sv
#    CommonLibSSE-NG places 'using namespace std::literals;' ONLY inside the
#    SKSE/RE/REL namespaces (bottom of SKSE/Impl/PCH.h), but the macro
#    expands at GLOBAL scope, where the unqualified 'sv' suffix is not
#    visible.
#    -> error C3688: invalid literal suffix 'sv'
#
# Fix: generate a wrapper PCH header at configure time that (a) includes
# SKSE/Impl/PCH.h and (b) adds the using-directive at global scope, then
# force-include it into EVERY translation unit (including the generated one)
# via target_precompile_headers.
set(VAW_PCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/VAWPrecompiledHeader.h")
file(WRITE "${VAW_PCH_FILE}" [[#include "SKSE/Impl/PCH.h"

using namespace std::literals;
]])
target_precompile_headers(${PROJECT_NAME} PRIVATE "${VAW_PCH_FILE}")

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
