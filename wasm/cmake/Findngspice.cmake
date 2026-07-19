# Findngspice.cmake - Stub for WASM builds
# The editor never links libngspice: the simulator engine runs in the
# ngspice_service worker (docs/features/ngspice-split/), and eeschema binds to
# the statically linked sharedspice CLIENT (wasm/stubs/sharedspice_client.cpp)
# in NGSPICE::init_dll()'s __EMSCRIPTEN__ branch.

if(EMSCRIPTEN OR NOT KICAD_SPICE)
    message(STATUS "ngspice: WASM build uses the sharedspice client header stub")

    # Set variables to indicate ngspice is "found"
    set(ngspice_FOUND TRUE)
    set(NGSPICE_FOUND TRUE)

    # Point at wasm/stubs/ngspice/sharedspice.h: the sharedspice types plus
    # the pcbjam_ngSpice_* client declarations. The library link line stays
    # empty — the engine lives in ngspice_service.wasm.
    set(NGSPICE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../stubs")
    set(NGSPICE_LIBRARY "")
    set(NGSPICE_LIBRARIES "")

    mark_as_advanced(NGSPICE_INCLUDE_DIR NGSPICE_LIBRARY)
else()
    # For non-WASM builds, use the system findngspice
    find_path(NGSPICE_INCLUDE_DIR ngspice/sharedspice.h)
    find_library(NGSPICE_LIBRARY NAMES ngspice)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(ngspice DEFAULT_MSG NGSPICE_LIBRARY NGSPICE_INCLUDE_DIR)
endif()
