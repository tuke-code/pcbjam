#!/bin/bash
# Open interactive shell in build container for debugging
set -e

cd "$(dirname "$0")/.."

# Pinned toolchain version (single source of truth) -> compose build.args needs it in the env.
source "$(dirname "$0")/../scripts/common/versions.sh"
export EMSCRIPTEN_VERSION

# Start container if not running
docker compose -f docker/docker-compose.yml up -d

# Open interactive shell
docker compose -f docker/docker-compose.yml exec kicad-wasm-builder bash
