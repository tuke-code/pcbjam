# WASM implementation of libcontext using Emscripten Asyncify
# Provides fiber/coroutine support for KiCad's router

set(LIBCONTEXT_WASM_SRCS
    libcontext_wasm.cpp
)

add_library(libcontext_wasm STATIC ${LIBCONTEXT_WASM_SRCS})

target_include_directories(libcontext_wasm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Asyncify is required for fiber support
# These flags must also be set on the final executable
target_compile_options(libcontext_wasm PRIVATE
    -pthread
)

target_link_options(libcontext_wasm INTERFACE
    -sASYNCIFY=1
    -sASYNCIFY_STACK_SIZE=65536
)
