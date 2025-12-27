# KiCad WebAssembly Port

Run KiCad PCBnew in the browser using WebAssembly.

## Project Structure

```
kicad-wasm/
├── kicad/                  # KiCad source (git submodule)
├── wxwidgets/              # wxWidgets source (git submodule)
├── wasm/                   # WASM compatibility layer
│   ├── kiplatform/         # Platform abstraction (app, UI, printing)
│   ├── libcontext/         # Coroutine/fiber implementation
│   ├── stubs/              # Stub implementations (libgit2, curl)
│   └── config/             # Build configuration headers
├── patches/                # KiCad source patches
├── scripts/                # Build scripts
│   ├── build-wxuniversal-wasm.sh   # Build wxWidgets for WASM
│   ├── build-wasm-test.sh          # Build wxWidgets test apps
│   ├── deps/               # Dependency build scripts
│   ├── kicad/              # KiCad build scripts
│   ├── common/             # Shared utilities and config
│   └── config/             # Build configuration
├── docker/                 # Docker build environment
├── tests/                  # Playwright E2E tests
├── output/                 # Build output (pcbnew.js, pcbnew.wasm)
└── docs/                   # Research documentation
```

## Two Build Workflows

### 1. KiCad Build (Docker)

Full KiCad PCBnew build using Docker:

```bash
# Build KiCad WASM
./docker/build.sh

# Copy output to test directory
./tests/scripts/setup-kicad-wasm.sh

# Run KiCad tests
cd tests && npm install && npm run test:kicad
```

Output: `output/pcbnew.js`, `output/pcbnew.wasm`

See [build.md](build.md) for detailed build documentation.

### 2. wxWidgets Test Apps (Local)

Build standalone wxWidgets test apps for feature testing:

```bash
# Build wxWidgets for WASM
./scripts/build-wxuniversal-wasm.sh

# Build test apps
./scripts/build-wasm-test.sh

# Run wxWidgets tests
cd tests && npm install && npm test
```

Output: `tests/wasm-app/standalone/`

## Prerequisites

### For KiCad Build (Docker)
- Docker Desktop with 16GB+ RAM allocated
- 10+ GB disk space for build cache

### For wxWidgets Build (Local)
- Emscripten SDK 4.0+
- Node.js 18+ (for tests)

```bash
# macOS
brew install emscripten node

# Initialize submodules
git submodule update --init --recursive
```

## Testing

```bash
cd tests
npm install

# Run all tests
npm test

# Run specific tests
npm run test:kicad          # KiCad tests only
npx playwright test menu    # Menu tests only
```

See [tests/README.md](tests/README.md) for test documentation.

## Current Status

- **wxWidgets WASM**: Core widgets working (menus, dialogs, grids, trees, OpenGL)
- **KiCad PCBnew**: Builds and loads in browser, canvas rendering working
- **In Progress**: Testing wxWidgets features used by KiCad

## Documentation

- [Build System](build.md) - Docker build details
- [Docker README](docker/README.md) - Container setup
- [Tests README](tests/README.md) - Test infrastructure
- [Research Docs](docs/) - Original research notes

## License

KiCad is GPL-3.0. This project follows the same license.
