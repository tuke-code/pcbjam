#!/bin/bash
# One-shot, idempotent provisioning for a freshly-created worktree.
#
# Builds/installs the test artifacts that are gitignored or NOT produced by the
# normal build pipeline (docker/build.sh, build-wx-wasm.sh, build-wasm-test.sh),
# and therefore do NOT carry into a new git worktree — the usual cause of
# "gal-webgl tests 404" and "collab specs fail: Could not resolve @pcbjam/shared":
#
#   1. host sysroot headers (boost etc.) copied out of the kicad-build-cache docker volume
#   2. the gal-webgl WASM test harness   -> tests/apps/gal-webgl/gal_webgl_test.{js,wasm}
#   3. the web/ pnpm workspace           -> so the collab bundle resolves @pcbjam/shared
#   4. the collab bundle                 -> tests/apps/kicad/collab-bundle.js
#
# Run it AFTER the wasm build (docker/build.sh + build-wx-wasm.sh) and after
# `cd tests && npm i`, so the docker deps volume, host build-wasm/wxwidgets and
# tests/node_modules already exist. Re-running skips steps whose outputs exist
# (use --force to rebuild everything).
#
# Usage:
#   ./scripts/setup-worktree.sh
#   ./scripts/setup-worktree.sh --force

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

FORCE=0
[ "${1:-}" = "--force" ] && FORCE=1

step() { printf '\n\033[1m==> %s\033[0m\n' "$1"; }
ok()   { printf '  \033[32m✓\033[0m %s\n' "$1"; }
warn() { printf '  \033[33m! %s\033[0m\n' "$1"; }

# 1. sysroot headers: deps (boost etc.) are built inside the kicad-build-cache
#    docker volume; the host-side gal build needs them under build-wasm/sysroot.
step "1/4 host sysroot headers (boost etc.)"
if [ "$FORCE" = 0 ] && [ -d build-wasm/sysroot/include/boost ]; then
    ok "build-wasm/sysroot/include/boost already present"
else
    BRANCH=$(git rev-parse --abbrev-ref HEAD | tr '/' '-' | tr '[:upper:]' '[:lower:]')
    VOL="kicad-wasm-${BRANCH}_kicad-build-cache"
    if ! docker volume inspect "$VOL" >/dev/null 2>&1; then
        # Fall back to any build-cache volume (single-worktree machines).
        VOL=$(docker volume ls -q --filter name=kicad-build-cache | head -1 || true)
    fi
    if [ -z "${VOL:-}" ]; then
        warn "no kicad-build-cache docker volume found — run 'docker/build.sh ... --build-deps' first; skipping"
    else
        echo "  volume: $VOL"
        mkdir -p build-wasm/sysroot
        docker run --rm -v "$VOL":/bw -v "$PWD/build-wasm/sysroot":/host alpine \
            sh -c 'cp -r /bw/sysroot/include /host/'
        ok "copied sysroot/include from $VOL"
    fi
fi

# 2. gal-webgl harness: not built by any normal pipeline target; needs the
#    sysroot headers (step 1) and a host wxWidgets build.
step "2/4 gal-webgl test harness"
if [ "$FORCE" = 0 ] && [ -f tests/apps/gal-webgl/gal_webgl_test.js ] && [ -f tests/apps/gal-webgl/gal_webgl_test.wasm ]; then
    ok "tests/apps/gal-webgl/gal_webgl_test.{js,wasm} already present"
elif [ ! -d build-wasm/sysroot/include/boost ]; then
    warn "sysroot headers missing (step 1 skipped) — skipping gal build"
elif [ ! -d build-wasm/wxwidgets/lib ]; then
    warn "wxWidgets not built — run 'scripts/build-wx-wasm.sh' first; skipping gal build"
else
    ./scripts/build-gal-webgl-test.sh
    ok "built gal-webgl harness"
fi

# 3. web/ workspace: provides @pcbjam/shared, which the collab bundle imports.
step "3/4 web/ pnpm workspace (collab @pcbjam/shared)"
if [ "$FORCE" = 0 ] && [ -d web/node_modules ]; then
    ok "web/node_modules already present"
elif ! command -v pnpm >/dev/null 2>&1; then
    warn "pnpm not found — run 'corepack enable' then re-run; skipping web install"
else
    ( cd "$PROJECT_ROOT/web" && pnpm install --frozen-lockfile )
    ok "installed web/ workspace"
fi

# 4. collab bundle: cheap esbuild; the collab specs also rebuild it in beforeAll,
#    but build it now so a plain run (or npx) has it present.
step "4/4 collab bundle"
if [ ! -d tests/node_modules ]; then
    warn "tests/node_modules missing — run 'cd tests && npm i' first; skipping collab bundle"
elif [ ! -d web/node_modules ]; then
    warn "web/ not installed (step 3 skipped) — skipping collab bundle"
else
    ( cd "$PROJECT_ROOT/tests" && node collab/build.mjs )
    ok "built tests/apps/kicad/collab-bundle.js"
fi

step "worktree setup complete"
echo "  (kicad app binaries are provisioned separately by 'cd tests && npm run setup:kicad')"
