# KiCad WebAssembly Port

Experimental project to run KiCad's core logic in WebAssembly.

## Project Structure

```
kicad-wasm/
├── kicad/          # KiCad source (git submodule)
├── patches/        # Patches to make deps optional
├── stubs/          # Stub headers for disabled deps
├── cmake/          # CMake modules
├── scripts/        # Build and maintenance scripts
├── core/           # Extracted core library for Wasm
└── docs/           # Knowledge base and plans
```

## Goals

1. **Phase 1**: Build KiCad with optional deps disabled (curl, git, OCC, ngspice)
2. **Phase 2**: Extract core computation code as standalone library
3. **Phase 3**: Compile core to WebAssembly
4. **Phase 4**: Run native GUI with Wasm worker backend
5. **Phase 5**: Browser-based UI

## Documentation

- [Implementation Plan](docs/IMPLEMENTATION_PLAN.md)
- [Knowledge Base (Summary)](docs/KNOWLEDGE_BASE.md)
- [Knowledge Base (Full)](docs/KNOWLEDGE_BASE_FULL.md)

## Design Decisions

- **Memory**: Serialize/deserialize on every operation (proof of concept)
- **Threading**: Web Workers (browser-first design)
- **Updates**: Full board re-serialization

## Getting Started

```bash
# Clone with submodules
git clone --recursive <repo-url>

# Apply patches
./scripts/prepare.sh

# Build with optional deps disabled
mkdir build && cd build
cmake .. -DKICAD_USE_CURL=OFF -DKICAD_USE_GIT=OFF -DKICAD_USE_OCC=OFF
make -j$(nproc)
```

## License

KiCad is GPL-3.0. This wrapper/tooling follows the same license.