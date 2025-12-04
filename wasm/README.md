# WASM Compatibility Layer

This directory contains WASM-specific implementations that allow KiCad to run in a web browser **without modifying KiCad's source code**.

## Principle

Instead of patching KiCad source files, we:
1. Override include paths to use our headers first
2. Provide alternative implementations for platform-specific code
3. Link our libraries instead of system libraries

## Directory Structure

```
wasm/
├── CMakeLists.txt          # Master CMake for compatibility layer
├── README.md               # This file
├── kiplatform/             # Platform abstraction implementations
│   ├── CMakeLists.txt
│   ├── app.cpp             # App lifecycle (paths, startup)
│   ├── drivers.cpp         # GPU detection (returns "WebGL")
│   ├── environment.cpp     # Environment variables (localStorage)
│   ├── io.cpp              # File I/O (WASM virtual filesystem)
│   ├── policy.cpp          # Security policy (always permissive)
│   ├── secrets.cpp         # Credential storage (localStorage)
│   ├── sysinfo.cpp         # System information
│   └── printing.cpp        # Print support (browser print())
├── libcontext/             # Coroutine/fiber implementation
│   ├── CMakeLists.txt
│   └── fcontext_wasm.cpp   # Emscripten Asyncify fibers
├── shims/                  # Header overrides
│   └── *.h                 # Headers that redirect to our impls
└── config/                 # Build configuration
    ├── kicad_wasm_config.h # Version and feature config
    └── setup.h             # Platform setup
```

## How It Works

### Include Path Override

When building KiCad for WASM, we add our directories first in the include path:

```bash
-I$PROJECT_ROOT/wasm/shims
-I$PROJECT_ROOT/wasm/kiplatform
-I$PROJECT_ROOT/stubs/include
```

This means when KiCad includes `<kiplatform/app.h>`, it finds our version first.

### Library Override

We build `libkiplatform_wasm.a` and link it instead of the native kiplatform:

```bash
-L$BUILD_ROOT/wasm -lkiplatform_wasm
```

### CMake Integration

The main KiCad build is configured to find our implementations:

```cmake
-DCMAKE_MODULE_PATH="$PROJECT_ROOT/cmake"
-DKIPLATFORM_LIBRARY="$BUILD_ROOT/wasm/libkiplatform_wasm.a"
```

## Adding New Implementations

1. Create the implementation file in the appropriate directory
2. Add it to the CMakeLists.txt
3. Ensure the header interface matches KiCad's expected interface
4. Test with a minimal build before full integration
