# FindKiplatformWASM.cmake
# Find module for WASM implementation of kiplatform
#
# This module is used instead of the platform-specific kiplatform
# when building KiCad for WebAssembly.
#
# This module defines:
#   KIPLATFORM_FOUND - System has kiplatform for WASM
#   KIPLATFORM_INCLUDE_DIRS - Include directories
#   KIPLATFORM_LIBRARIES - Libraries to link
#   KIPLATFORM_SOURCES - Source files to compile

include(FindPackageHandleStandardArgs)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "FindKiplatformWASM is only for Emscripten builds")
endif()

# Get the directory containing this file
get_filename_component(_FIND_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(KIPLATFORM_WASM_DIR "${_FIND_DIR}/../kiplatform" ABSOLUTE)

# Define source files
set(KIPLATFORM_SOURCES
    ${KIPLATFORM_WASM_DIR}/app.cpp
    ${KIPLATFORM_WASM_DIR}/drivers.cpp
    ${KIPLATFORM_WASM_DIR}/environment.cpp
    ${KIPLATFORM_WASM_DIR}/io.cpp
    ${KIPLATFORM_WASM_DIR}/policy.cpp
    ${KIPLATFORM_WASM_DIR}/secrets.cpp
    ${KIPLATFORM_WASM_DIR}/sysinfo.cpp
    ${KIPLATFORM_WASM_DIR}/ui.cpp
)

# Include directory for platform headers
# We use KiCad's own headers, just provide our implementation
set(KIPLATFORM_INCLUDE_DIRS ${KIPLATFORM_WASM_DIR})

# No prebuilt library - sources are compiled directly into KiCad
set(KIPLATFORM_LIBRARIES "")

# Mark as found
set(KIPLATFORM_FOUND TRUE)

find_package_handle_standard_args(KiplatformWASM
    REQUIRED_VARS KIPLATFORM_SOURCES KIPLATFORM_INCLUDE_DIRS
)

mark_as_advanced(KIPLATFORM_SOURCES KIPLATFORM_INCLUDE_DIRS KIPLATFORM_LIBRARIES)
