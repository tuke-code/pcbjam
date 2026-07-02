#!/bin/bash
# Build the merged KiCad editor (pcbnew + eeschema kifaces in ONE image) for
# WebAssembly. The editor frame (PCB / Footprint / Schematic / Symbol) is chosen
# at runtime via single_top.cpp's --frame flag (editor-unification Part 2).
# Thin wrapper around build-kicad-target.sh — see that script for options.

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/build-kicad-target.sh" kicad_editor "$@"
