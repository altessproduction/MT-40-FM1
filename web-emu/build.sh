#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# MT-40X web emulator: firmware sources -> wasm32 (system clang/lld, or zig cc),
# then copy the web shell into dist/ and refresh docs/dub/ (the GitHub Pages copy).
#   sh web-emu/build.sh
# CC: any wasm32-capable driver works — clang, or `python3 -m ziglang cc`
# (CC_FLAGS then needs -target wasm32-freestanding; zig 0.17 refuses the bare wasm32).
set -e
cd "$(dirname "$0")"
ROOT=$(cd .. && pwd)

if [ -z "$CC" ]; then
    if command -v clang >/dev/null 2>&1; then
        CC=clang
        TARGET=wasm32
    else
        CC="python3 -m ziglang cc"
        TARGET=wasm32-freestanding
    fi
fi
CFLAGS=${CFLAGS:--target $TARGET -O2 -fno-builtin -ffreestanding -nostdlib \
    -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
    -Wno-date-time}
LDFLAGS=${LDFLAGS:--Wl,--no-entry -Wl,--export-memory \
    -Wl,-z,stack-size=1048576 -Wl,--global-base=1048576 -Wl,--strip-all}

test -f "$ROOT/build/gen/felucca_tables.h" || {
    echo "generated headers missing; run tools/gen_tables.py et al. into build/gen first" >&2
    exit 1
}

mkdir -p dist
$CC $CFLAGS -I "$ROOT/build/gen" -I "$ROOT/firmware/src" -I "$ROOT/firmware/bsp/chip" \
    $LDFLAGS -o dist/mt40x.wasm mt40x_wasm.c

cp index.html emu.js worklet.js manifest.webmanifest dist/
# Keep the GitHub Pages bundle in sync with the freshly compiled firmware.
if [ -d "$ROOT/docs/dub" ]; then
    cp dist/mt40x.wasm "$ROOT/docs/dub/mt40x.wasm"
    cp index.html emu.js worklet.js manifest.webmanifest "$ROOT/docs/dub/"
fi
ls -la dist/
