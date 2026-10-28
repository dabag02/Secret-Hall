#!/usr/bin/env bash
# Rete di sicurezza della migrazione (passo M0).
#   tools/regress.sh baseline   compila e salva gli screenshot di riferimento in tests/baseline/
#   tools/regress.sh check      compila, genera tests/current/ e confronta con il riferimento
# Flusso: prima di ogni passo `baseline` (dal codice che funziona), dopo il passo `check`.
# I riferimenti dipendono dalla macchina (versione di Mesa): si generano in locale, non si condividono.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
"$ROOT/tools/build.sh" "$ROOT/build"
case "${1:-check}" in
  baseline) rm -rf "$ROOT/tests/baseline"; "$ROOT/tools/shots.sh" "$ROOT/build/sala_segreta" "$ROOT/tests/baseline" ;;
  check)    [ -d "$ROOT/tests/baseline" ] || { echo "Manca tests/baseline: esegui prima 'baseline'." >&2; exit 2; }
            rm -rf "$ROOT/tests/current"; "$ROOT/tools/shots.sh" "$ROOT/build/sala_segreta" "$ROOT/tests/current"
            python3 "$ROOT/tools/compare.py" "$ROOT/tests/baseline" "$ROOT/tests/current" "${@:2}" ;;
  *) echo "uso: tools/regress.sh baseline|check [--tol N]" >&2; exit 2 ;;
esac
