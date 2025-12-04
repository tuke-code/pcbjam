# WASM implementation of kiplatform
# Provides WASM-specific implementations of KiCad's platform abstraction layer

set(KIPLATFORM_WASM_SRCS
    app.cpp
    drivers.cpp
    environment.cpp
    io.cpp
    policy.cpp
    secrets.cpp
    sysinfo.cpp
    ui.cpp
)

add_library(kiplatform_wasm STATIC ${KIPLATFORM_WASM_SRCS})

target_include_directories(kiplatform_wasm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${PROJECT_ROOT}/kicad/libs/kiplatform/include
)

# Link with wxWidgets
target_link_libraries(kiplatform_wasm
    # wxWidgets will be added during main build
)
