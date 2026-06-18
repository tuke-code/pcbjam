#!/bin/bash
# Build the standalone .lib -> .kicad_sym converter (sym_convert) for WebAssembly,
# as a node CLI. Thin wrapper around build-kicad-target.sh — see that script for options.
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/build-kicad-target.sh" sym_convert "$@"
