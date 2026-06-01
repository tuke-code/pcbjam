#!/bin/bash
# Build KiCad pl_editor (drawing-sheet editor) for WebAssembly.
# Thin wrapper around build-kicad-target.sh — see that script for options.

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/build-kicad-target.sh" pl_editor "$@"
