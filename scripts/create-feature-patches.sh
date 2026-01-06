#!/bin/bash
# Usage: ./scripts/create-feature-patches.sh [branch-name]
# Creates patches in features/<branch-name>/

set -e

BRANCH=${1:-$(git branch --show-current)}
FEATURE_DIR="features/${BRANCH}"

mkdir -p "$FEATURE_DIR"

# Root repo patch (exclude submodules)
git diff HEAD -- ':!kicad' ':!wxwidgets' > "$FEATURE_DIR/root.patch"

# Submodule patches (diff from upstream base)
KICAD_BASE=$(git -C kicad log --format='%H' --author-not='viktor.vaczi@emergence-engineering.com' --author-not='noreply@anthropic.com' -1)
git -C kicad diff $KICAD_BASE > "$FEATURE_DIR/kicad.patch"

WX_BASE="v3.2.6"
git -C wxwidgets diff $WX_BASE > "$FEATURE_DIR/wxwidgets.patch"

echo "Patches created in $FEATURE_DIR/"
ls -la "$FEATURE_DIR"/*.patch 2>/dev/null || echo "No patches generated"
