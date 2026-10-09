#!/usr/bin/env bash
# Build Nut65 Vial keymaps in parallel. Run inside QMK MSYS from the vial-qmk
# checkout: tools/build_vial.sh [keymap ...]   (default: vial vial_personal)
#
# QMK's top-level Makefile starts the real build as a sub-make that make can't
# see is recursive, so -j never reaches it ("jobserver unavailable: using -j1").
# Calling builddefs/build_keyboard.mk directly gets every core working.
# SKIP_GIT skips git queries for the version string, which are slow on Windows
# with vial-qmk's 25k files and submodules; Vial doesn't use that string.
set -euo pipefail

jobs=$(nproc)
keymaps=("$@")
[ $# -eq 0 ] && keymaps=(vial vial_personal)

for km in "${keymaps[@]}"; do
    echo "== leku/nut65:$km (-j$jobs)"
    start=$SECONDS
    make -j"$jobs" -r -R -f builddefs/build_keyboard.mk KEYBOARD=leku/nut65 KEYMAP="$km" QMK_BIN=qmk COLOR=false SILENT=false SKIP_GIT=yes
    echo "== $km built in $((SECONDS - start)) s"
done
