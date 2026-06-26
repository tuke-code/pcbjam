// Minimal native WebAssembly exceptions reproduction for the Safari/WebKit -fwasm-exceptions
// startup-crash issue (emscripten #25365). The whole point is to be the SMALLEST possible wasm that
// is compiled with -fwasm-exceptions and exercises a real C++ throw/catch — no asyncify, no pthreads,
// no embind — so a Safari load failure can ONLY be the native-EH codegen, nothing else.
//
// Built in two encodings (see build.sh):
//   repro-legacy.*  -sWASM_LEGACY_EXCEPTIONS=1  (what KiCad-WASM ships today)
//   repro-new.*     -sWASM_LEGACY_EXCEPTIONS=0  (the standardized exnref encoding)
//
// main() throws + catches and stashes the result on globalThis.__ehResult; if the module fails to
// instantiate (the Safari crash) the page's loader sets globalThis.__ehError instead.
#include <cstdio>
#include <stdexcept>
#include <emscripten.h>

int main() {
    int code = 0;
    try {
        throw std::runtime_error("boom");
    } catch (const std::exception& e) {
        printf("caught: %s\n", e.what());
        code = 42;
    }
    EM_ASM({ globalThis.__ehResult = "EH_OK code=" + $0; }, code);
    printf("EH_OK code=%d\n", code);
    return 0;
}
