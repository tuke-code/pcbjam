#!/usr/bin/env bash
# Deploy the heavy Gerber-viewer binaries to Cloudflare R2.
#
# Prereqs (one-time, done by you — see the chat guide):
#   1. R2 enabled on your Cloudflare account (dashboard; needs a payment method).
#   2. `wrangler login`  (browser auth).
#
# This script then (idempotent — safe to re-run after a rebuild):
#   - creates the bucket (skips if it already exists)
#   - sets CORS (site/scripts/r2-cors.json)
#   - enables the public r2.dev URL and prints it
#   - gzips gerbview.wasm and uploads it (Content-Encoding: gzip) + kicad-resources.bin
#
# Usage:  site/scripts/r2-deploy.sh [bucket-name]      (default: pcbjam-assets)
set -euo pipefail

BUCKET="${1:-pcbjam-assets}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # = site/
SRC="$ROOT/public/gerber-demo/wasm"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

command -v wrangler >/dev/null || { echo "wrangler not found (npm i -g wrangler)"; exit 1; }
wrangler whoami >/dev/null 2>&1 || { echo "Not logged in. Run: wrangler login"; exit 1; }
[[ -f "$SRC/gerbview.wasm" ]]        || { echo "Missing $SRC/gerbview.wasm — run scripts/sync-demo-wasm.sh"; exit 1; }
[[ -f "$SRC/kicad-resources.bin" ]]  || { echo "Missing $SRC/kicad-resources.bin — run scripts/sync-demo-wasm.sh"; exit 1; }

echo "==> Bucket: $BUCKET"
wrangler r2 bucket create "$BUCKET" 2>/dev/null \
  && echo "    created" \
  || echo "    already exists (continuing)"

echo "==> CORS"
wrangler r2 bucket cors set "$BUCKET" --file "$ROOT/scripts/r2-cors.json"

echo "==> Public r2.dev URL (makes the bucket's objects publicly readable)"
wrangler r2 bucket dev-url enable "$BUCKET" || true
wrangler r2 bucket dev-url get "$BUCKET" || true

echo "==> gzip gerbview.wasm"
gzip -9 -c "$SRC/gerbview.wasm" > "$TMP/gerbview.wasm.gz"
echo "    $(du -h "$SRC/gerbview.wasm" | cut -f1) -> $(du -h "$TMP/gerbview.wasm.gz" | cut -f1) gzipped"

echo "==> Upload gerbview.wasm (gzip, application/wasm)"
wrangler r2 object put "$BUCKET/gerbview.wasm" --remote \
  --file "$TMP/gerbview.wasm.gz" \
  --content-type "application/wasm" \
  --content-encoding "gzip" \
  --cache-control "public, max-age=3600"

echo "==> Upload kicad-resources.bin (octet-stream, no encoding)"
wrangler r2 object put "$BUCKET/kicad-resources.bin" --remote \
  --file "$SRC/kicad-resources.bin" \
  --content-type "application/octet-stream" \
  --cache-control "public, max-age=3600"

echo ""
echo "Done. Copy the r2.dev URL printed above (https://<hash>.r2.dev) into"
echo "site/public/gerber-demo/boot.js -> R2_BASE, then redeploy the site."
echo "(Or bind a custom domain: wrangler r2 bucket domain add $BUCKET --domain assets.pcbjam.com)"