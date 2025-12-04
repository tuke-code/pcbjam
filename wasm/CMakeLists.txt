# WASM Compatibility Layer for KiCad
# This directory contains WASM-specific implementations that replace
# platform-specific code in KiCad without modifying KiCad's source.

cmake_minimum_required(VERSION 3.22)
project(kicad_wasm_compat)

# Only build for Emscripten
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This project is only for Emscripten builds")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/shims
)

# kiplatform WASM implementation
add_subdirectory(kiplatform)

# libcontext Asyncify fiber implementation
add_subdirectory(libcontext)
