#!/usr/bin/env bash
# Genera gli screenshot di riferimento (deterministici) in una cartella.
# Uso: tools/shots.sh <eseguibile> <cartella-output>
# Usa il renderer software di Mesa dentro Xvfb: indipendente dalla GPU, quindi riproducibile.
# Richiede: xvfb-run, python3 con Pillow (pacman -S xorg-server-xvfb python-pillow).
set -euo pipefail
BIN="${1:?eseguibile}"; OUT="${2:?cartella}"; mkdir -p "$OUT"
export LIBGL_ALWAYS_SOFTWARE=1

# nome|argomenti  (ogni scenario sceglie una vista e uno stato di gioco con le opzioni di test)
SCENES=(
"01_stanza_iniziale|-3.6 3.6 45 -7 --skipdice"
"02_dadi_in_volo|--dicetime 0.9"
"03_dadi_fermi|--dicetime 5"
"04_inventario_demo|-3.6 3.6 45 -7 --demo --inv --fig 160"
"05_armatura_indossata|-3.6 3.6 45 -7 --demo --inv --worn --fig 20"
"06_oggetti_in_mano|-3.6 3.6 30 5 --demo"
"07_camino_acceso|2.8 2.2 90 -15 --skipdice --fire"
"08_tavolo_in_fiamme|-2.6 3.0 20 -10 --skipdice --burn"
"09_tavolo_bruciato_scheletro|1.9 1.7 -8 -14 --skipdice --burnt"
"10_scrigno_aperto|-0.2 1.7 0 -32 --skipdice --open"
"11_rastrelliera_in_fiamme|-2.0 1.2 -90 -15 --skipdice --rackburn"
"12_rastrelliera_crollata_veleno|-2.0 1.2 -90 -15 --skipdice --rackburnt --hp 9 --poison 6"
"13_menu_secchio|1.0 -2.2 20 -25 --skipdice --menu"
"14_oggetto_trasportato|1.0 -2.2 20 -25 --skipdice --carry"
"15_porta_magica_scarica|3.6 3.4 90 0 --skipdice --zap"
"16_morte|-3.6 3.6 45 -7 --skipdice --dead"
"17_interazione_E|1.0 -3.4 0 -4 --skipdice --presse"
"18_scrigno_menu_chiave|-0.2 1.7 0 -32 --skipdice --demo --presse"
"19_scrigno_contenuto_E|-0.2 1.7 0 -32 --skipdice --open --presse"
)
for sc in "${SCENES[@]}"; do
  name="${sc%%|*}"; args="${sc#*|}"
  # shellcheck disable=SC2086
  xvfb-run -a -s "-screen 0 1280x720x24" "$BIN" --shot "$OUT/$name.ppm" $args >/dev/null 2>&1 || { echo "ERRORE nello scenario $name" >&2; exit 1; }
done
python3 "$(dirname "$0")/compare.py" --topng "$OUT"
echo "screenshot generati in: $OUT (${#SCENES[@]} scenari)"
