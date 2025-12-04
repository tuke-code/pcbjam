# FindLibcontextWASM.cmake
# Find module for WASM implementation of libcontext using Asyncify
#
# This module replaces the platform-specific libcontext assembly
# when building KiCad for WebAssembly.
#
# This module defines:
#   LIBCONTEXT_FOUND - System has libcontext for WASM
#   LIBCONTEXT_INCLUDE_DIRS - Include directories
#   LIBCONTEXT_LIBRARIES - Libraries to link (none for WASM)
#   LIBCONTEXT_SOURCES - Source files to compile

include(FindPackageHandleStandardArgs)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "FindLibcontextWASM is only for Emscripten builds")
endif()

# Get the directory containing this file
get_filename_component(_FIND_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(LIBCONTEXT_WASM_DIR "${_FIND_DIR}/../libcontext" ABSOLUTE)

# Define source files
set(LIBCONTEXT_SOURCES
    ${LIBCONTEXT_WASM_DIR}/libcontext_wasm.cpp
)

# Include directories
set(LIBCONTEXT_INCLUDE_DIRS ${LIBCONTEXT_WASM_DIR})

# No prebuilt library
set(LIBCONTEXT_LIBRARIES "")

# Mark as found
set(LIBCONTEXT_FOUND TRUE)

find_package_handle_standard_args(LibcontextWASM
    REQUIRED_VARS LIBCONTEXT_SOURCES LIBCONTEXT_INCLUDE_DIRS
)

mark_as_advanced(LIBCONTEXT_SOURCES LIBCONTEXT_INCLUDE_DIRS LIBCONTEXT_LIBRARIES)
