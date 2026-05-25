#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/build"
UI_DIR="$ROOT/ui"
JOBS="${JOBS:-$(nproc)}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

log() { printf '\033[1;34m==> %s\033[0m\n' "$*"; }
err() { printf '\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# ── Parse flags ───────────────────────────────────────────────────────────────
SKIP_BACKEND=0
SKIP_UI=0
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        --backend)    SKIP_UI=1 ;;
        --ui)         SKIP_BACKEND=1 ;;
        --clean)      CLEAN=1 ;;
        --debug)      BUILD_TYPE=Debug ;;
        --help|-h)
            echo "Usage: $0 [--backend] [--ui] [--clean] [--debug]"
            echo "  (no flags) build both backend and UI"
            exit 0 ;;
        *) err "Unknown flag: $arg" ;;
    esac
done

# ── Clean ─────────────────────────────────────────────────────────────────────
if [[ $CLEAN -eq 1 ]]; then
    log "Cleaning build artifacts"
    rm -rf "$BUILD_DIR"
    rm -rf "$UI_DIR/dist"
fi

# ── Backend ───────────────────────────────────────────────────────────────────
if [[ $SKIP_BACKEND -eq 0 ]]; then
    log "Configuring backend (CMake, $BUILD_TYPE)"
    cmake -S "$ROOT" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -Wno-dev

    log "Building backend (-j$JOBS)"
    cmake --build "$BUILD_DIR" --parallel "$JOBS"

    echo "  Binary: $BUILD_DIR/avenger"
fi

# ── UI ────────────────────────────────────────────────────────────────────────
if [[ $SKIP_UI -eq 0 ]]; then
    [[ -d "$UI_DIR/node_modules" ]] || {
        log "Installing UI dependencies (npm install)"
        npm --prefix "$UI_DIR" install
    }

    log "Building UI (vite)"
    npm --prefix "$UI_DIR" run build

    echo "  Assets: $UI_DIR/dist"
fi

log "Done"
