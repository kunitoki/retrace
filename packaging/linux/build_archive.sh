#!/usr/bin/env bash
# =============================================================================
# Build a Linux tarball (.tar.gz) containing every built plugin format and the
# standalone application, with its .desktop entry.
#
# Usage: packaging/linux/build_archive.sh [CONFIG] [BUILD_DIR (default: build/linux)]
# =============================================================================
set -euo pipefail

CONFIG="${1:-${CONFIG:-Release}}"
BUILD_DIR="${2:-${BUILD_DIR:-}}"
BUILD_PLATFORM="${BUILD_PLATFORM:-linux}"
source "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/common.sh"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$DIST_DIR"

ROOT="$STAGE/$PRODUCT_SLUG-$VERSION"
mkdir -p "$ROOT"

declare -a FOUND=()
stage_bundle() {
    local ext="$1" dest="$2"
    local artifact
    artifact="$(find_artifact "$ext")"
    if [[ -z "$artifact" ]]; then
        warn "No .$ext artifact found for $CONFIG; skipping."
        return
    fi
    log "Staging $(basename "$artifact") -> $dest"
    mkdir -p "$ROOT/$dest"
    cp -R "$artifact" "$ROOT/$dest/"
    FOUND+=("$ext")
}

stage_bundle vst3 "vst3"
stage_bundle clap "clap"
stage_bundle lv2  "lv2"

# The standalone executable on Linux is a plain file (no bundle), named after
# the product. It gets a launcher-friendly slug and a .desktop entry.
if [[ -f "$PROJECT_ROOT/assets/app-icon.png" ]]; then
    mkdir -p "$ROOT/share/icons"
    cp "$PROJECT_ROOT/assets/app-icon.png" "$ROOT/share/icons/$PRODUCT_SLUG.png"
fi

stage_executable() {
    local name="$1" description="$2" kind="$3"
    local executable slug
    executable="$(find_executable "$name")"
    if [[ -z "$executable" ]]; then
        warn "No $kind executable '$name' found for $CONFIG; skipping."
        return
    fi
    slug="$(slugify "$name")"
    log "Staging $kind '$name' -> bin/$slug"
    mkdir -p "$ROOT/bin"
    cp "$executable" "$ROOT/bin/$slug"
    chmod 755 "$ROOT/bin/$slug"
    write_desktop_entry "$name" "$description" "$slug" "$PRODUCT_SLUG" \
        "$ROOT/share/applications/$slug.desktop"
    FOUND+=("$kind")
}

stage_executable "$PRODUCT_NAME" "$PRODUCT_DESCRIPTION" "standalone application"

[[ ${#FOUND[@]} -gt 0 ]] || die "No artifacts found under $BUILD_DIR for config $CONFIG. Build the project first."

[[ -f "$PROJECT_ROOT/LICENSE" ]] && cp "$PROJECT_ROOT/LICENSE" "$ROOT/LICENSE"

ARCH="$(uname -m)"
OUTPUT="$DIST_DIR/$PRODUCT_SLUG-$VERSION-linux-$ARCH.tar.gz"
log "Creating archive"
tar -czf "$OUTPUT" -C "$STAGE" "$PRODUCT_SLUG-$VERSION"
log "Created $OUTPUT"
