#!/usr/bin/env bash
# Compila il gioco (Makefile incrementale; passo M2). Uso: tools/build.sh [cartella-output]   (default: build/)
# In alternativa: cmake -S . -B build && cmake --build build
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/build}"
make -C "$ROOT" -j"$(nproc)" BUILD="$OUT"
echo "compilato: $OUT/sala_segreta"
