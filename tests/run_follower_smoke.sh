#!/usr/bin/env bash
# Host simulation only. For a real GBA build use: make -j"$(nproc)" firered_rev1
set -euo pipefail
cd "$(dirname "$0")/.."
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT
COMMON=(-O0 -std=gnu11 -ffunction-sections -fdata-sections -Wno-deprecated-non-prototype -iquote include -DFIRERED -DREVISION=1 -DENGLISH -DMODERN=0 '-D_(x)=x')
clang "${COMMON[@]}" '-DINCBIN_U32(x)={0}' '-DINCBIN_U16(x)={0x0000,0x001F,0x03E0,0x7C00,0x031F,0x7FE0,0x4210,0x14A5,0x56B5,0x3DEF,0x2108,0x6318,0x5294,0x1084,0x35AD,0x77BD}' -c src/following_pokemon.c -o "$TMP_DIR/follower.o"
clang "${COMMON[@]}" -c tests/follower_runtime_smoke.c -o "$TMP_DIR/mock.o"
clang -Wl,--gc-sections "$TMP_DIR/follower.o" "$TMP_DIR/mock.o" -o "$TMP_DIR/follower_runtime_smoke"
"$TMP_DIR/follower_runtime_smoke"
