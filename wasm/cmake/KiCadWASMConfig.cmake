# KiCad WASM Configuration
# This file configures KiCad to use WASM-specific implementations

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "KiCadWASMConfig.cmake is only for Emscripten builds")
endif()

# Set the path to WASM compatibility layer
get_filename_component(KICAD_WASM_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

message(STATUS "KiCad WASM compatibility layer: ${KICAD_WASM_DIR}")

# Add include directories for WASM implementations
# These will be searched BEFORE KiCad's own includes, allowing us to
# provide our own implementations without modifying KiCad source
set(KICAD_WASM_INCLUDE_DIRS
    ${KICAD_WASM_DIR}/kiplatform
    ${KICAD_WASM_DIR}/libcontext
    ${KICAD_WASM_DIR}/config
    ${KICAD_WASM_DIR}/shims
)

# Define macro to indicate we're using WASM platform
add_definitions(-DKICAD_PLATFORM_WASM=1)
add_definitions(-D__WXUNIVERSAL__=1)

# Emscripten-specific compile options
add_compile_options(
    -pthread
    -sUSE_PTHREADS=1
)

# Emscripten-specific link options
add_link_options(
    -pthread
    -sUSE_PTHREADS=1
    -sPTHREAD_POOL_SIZE=4
    -sASYNCIFY=1
    -sASYNCIFY_STACK_SIZE=65536
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=256MB
    -sMAXIMUM_MEMORY=4GB
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sENVIRONMENT=web,worker
)

# Export variables for use by parent CMakeLists
set(KICAD_WASM_INCLUDE_DIRS ${KICAD_WASM_INCLUDE_DIRS} PARENT_SCOPE)
set(KICAD_WASM_DIR ${KICAD_WASM_DIR} PARENT_SCOPE)
