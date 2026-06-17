#!/usr/bin/env bash
# Mirror the gerbview WASM build outputs into the blog demo's local asset dir.
#
# Phase A (local verification) serves the viewer from these files. They are
# git-ignored (see site/.gitignore): the 52 MB gerbview.wasm must never be
# committed or shipped to Vercel — in production the heavy binaries live on
# Cloudflare R2 (see the blog-demo plan, Phase B). Re-run this after rebuilding
# gerbview to refresh the local copy.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/output"
DST="$ROOT/site/public/gerber-demo/wasm"

mkdir -p "$DST"
for f in wx.js wx-dom.js gerbview.js gerbview.wasm images.tar.gz; do
  if [[ ! -f "$SRC/$f" ]]; then
    echo "ERROR: missing $SRC/$f — build gerbview first (docker/build.sh)." >&2
    exit 1
  fi
done

cp "$SRC/wx.js"         "$DST/wx.js"
cp "$SRC/wx-dom.js"     "$DST/wx-dom.js"
cp "$SRC/gerbview.js"   "$DST/gerbview.js"
cp "$SRC/gerbview.wasm" "$DST/gerbview.wasm"
# Store the resources tarball WITHOUT a .gz extension: dev servers (Vite/sirv)
# and some CDNs auto-add `Content-Encoding: gzip` for `*.gz`, which makes the
# browser pre-decompress it — then KiCad's own gunzip fails. boot.js fetches
# this name and writes it into MEMFS as images.tar.gz.
cp "$SRC/images.tar.gz" "$DST/kicad-resources.bin"

for f in wx.js wx-dom.js gerbview.js gerbview.wasm kicad-resources.bin; do
  echo "synced $f ($(du -h "$DST/$f" | cut -f1))"
done
echo "Local demo WASM ready at $DST"
