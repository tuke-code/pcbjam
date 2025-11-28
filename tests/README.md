# KiCad WASM Tests

Playwright tests for verifying the wxWidgets WASM port.

## Prerequisites

- Node.js 18+
- Emscripten SDK (for building)

## Building the Test App

```bash
../scripts/build-wasm-test.sh
```

This builds `wasm-app/minimal_test.{html,js,wasm}`.

## Running Tests

```bash
npm install
npm test
```

## Viewing the App Directly

Start a local server in the wasm-app directory:

```bash
cd wasm-app
npx serve .
```

Then open http://localhost:3000/minimal_test.html in your browser.

Alternative using Python:

```bash
cd wasm-app
python3 -m http.server 8000
```

Then open http://localhost:8000/minimal_test.html
