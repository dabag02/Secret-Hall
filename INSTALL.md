# INSTALL — La Sala Segreta

Come ottenere le dipendenze su **Arch Linux**, compilare e avviare il gioco. Aggiornato allo stato **v0.7.8 (migrazione M4c completata)**.
Per la storia e lo stato del progetto: `changelog.md` · per le regole di gioco: `rules.md` · per l'architettura: `architecture.md`.

---

## 1. Albero del progetto

```
sala_segreta/
├─ sala_segreta.cpp            Logica di interazione, dadi e main (969 righe). Si assottiglia nei passi M4–M8
├─ Makefile                    Build incrementale (consigliata): `make -j$(nproc)` → build/sala_segreta
├─ CMakeLists.txt              Build alternativa con CMake/Ninja
├─ INSTALL.md                  Questo file
├─ changelog.md                Stato attuale + cronologia degli sviluppi (da leggere per primo in una nuova chat)
├─ rules.md                    Regole generali del progetto con ID (A1…R6), specifiche e decisioni aperte
├─ architecture.md             Analisi del codice, architettura modulare, roadmap di migrazione M0–M12 e funzioni F1–F21
│
├─ src/                        Moduli estratti dal monolite (dal passo M1)
│  ├─ core/                    Livello base: nessuna dipendenza da OpenGL/SDL
│  │  ├─ types.h               PI, tipo V (vettore 3D) e operazioni vettoriali
│  │  └─ config.h              Costanti condivise (sala 10×4 m, raggio giocatore, 20 kg, torce, peso scheletro)
│  ├─ game/                    Dati e regole senza GL/SDL (dal passo M4a)
│  │  ├─ defs.h/.cpp           DefId, ObjectDef, MaterialDef: nome, peso, materiale, flag, visualId degli oggetti
│  │  └─ entity.h              Entity: l'unico concetto di oggetto (in mano, nell'inventario o a terra)
│  ├─ gfx/                     Primitive grafiche (OpenGL a pipeline fissa)
│  │  ├─ primitives.h/.cpp     PM, col, line, box, cone, dome, arc, ring*, …
│  ├─ render/                  Disegno della scena (dal passo M7 un backend sostituibile)
│  │  ├─ render.h              Dichiarazioni di tutte le funzioni di disegno
│  │  ├─ room.cpp              Pareti a blocchi, pavimento, travi, porte, quadri, scarica delle porte magiche
│  │  ├─ wall_props.cpp        Camino (con fuoco), spada, scudo e ganci, torce a muro
│  │  ├─ models.cpp            Elmo e pezzi d'armatura, gemma, monete, libro, cenere, chiave, fiamma, torcia, secchio
│  │  ├─ rack.cpp              Rastrelliera: integra, in fiamme, carbonizzata
│  │  ├─ furniture.cpp         Tavolo (integro/in fiamme/bruciato), sedia, scrigno
│  │  ├─ skeleton.cpp          Scheletro seduto
│  │  ├─ scene.cpp             Oggetti sciolti e trasportati, drawWorld
│  │  ├─ lighting.cpp          Luci per vertice (setLights) e camera (lookAt)
│  │  └─ viewmodel.cpp         Oggetti tenuti in mano in prima persona
│  ├─ ui/                      Interfaccia 2D (testo, inventario, HUD)
│  │  ├─ text.h/.cpp           TextCache: testo UTF-8 → texture (SDL_ttf), riquadri di messaggio
│  │  ├─ draw2d.h/.cpp         fillRect / strokeRect
│  │  ├─ figure.h/.cpp         Uomo wireframe ruotabile (tunica, calzamaglia, sandali, armatura indossata)
│  │  ├─ inventory_ui.h/.cpp   Pannello inventario: griglia, mani, equipaggiamento, statistiche, pesi, icone
│  │  └─ hourglass.h/.cpp      Clessidra animata dei conti alla rovescia (Countdown, drawCountdowns)
│  ├─ items.h / items.cpp      Inventario, mani, equipaggiamento, statistiche, pesi, nomi (interim fino a M4d–M5)
│  ├─ state.h / state.cpp      Stato condiviso tra logica e disegno (interim fino al passo M5: GameState)
│  └─ skeleton_physics.h/.cpp  Fisica dello scheletro: posa delle braccia (IK / doppio pendolo)
│
├─ tools/                      Strumenti di sviluppo e di regressione visiva
│  ├─ build.sh                 Compila con make (equivale a `make -j`) in build/ o in una cartella a scelta
│  ├─ shots.sh                 Genera 19 screenshot deterministici (scenari di prova) con Xvfb + Mesa software
│  ├─ compare.py               Confronta due cartelle di screenshot pixel per pixel (immagine delle differenze)
│  └─ regress.sh               `baseline` salva i riferimenti, `check` confronta dopo una modifica
│
├─ archive/
│  └─ sala_segreta_v0.7.cpp    Copia del codice v0.7 prima della migrazione (per tornare indietro)
│
├─ tests/
│  ├─ unit/test_defs.cpp       Test headless (senza SDL/GL): `make test`
│  └─ baseline/, current/      Creati da regress.sh (riferimenti locali, non condivisi)
└─ build/                      Creata dalla build: eseguibile sala_segreta e oggetti intermedi (non va versionata)
```
Struttura prevista nei prossimi passi (M3 in poi): `src/ui/`, `src/game/`, `src/systems/`, `data/`: vedi `architecture.md`, sezione 4.

---

## 2. Dipendenze su Arch Linux

### Per compilare e giocare (obbligatorie)
```bash
sudo pacman -S --needed base-devel pkgconf sdl2 sdl2_ttf libglvnd fontconfig urw-base35-fonts
```
| Pacchetto | A cosa serve |
|---|---|
| `base-devel` | compilatore `g++` (C++17), `make` e strumenti di build |
| `pkgconf` | trova le opzioni di compilazione di SDL2 (`pkg-config`) |
| `sdl2` | finestra, input (tastiera/mouse), contesto OpenGL. Fornisce `sdl2-config` |
| `sdl2_ttf` | testo a schermo (messaggi, inventario) con font TrueType/OpenType |
| `libglvnd` | intestazioni e libreria `libGL` (OpenGL) usate da `-lGL` |
| `fontconfig` | ricerca del font (`fc-match`) |
| `urw-base35-fonts` | contiene **P052**, il clone libero di Palatino/Book Antiqua usato per il testo |

**Driver grafico:** serve un driver OpenGL funzionante. Con la RTX 3070 è sufficiente `nvidia`/`nvidia-utils` (o `nvidia-open`/`nvidia-open-dkms`) già installato; con GPU AMD/Intel, `mesa`. Non occorre altro: il gioco usa OpenGL a pipeline fissa (compatibilità), supportata da tutti.

**Font alternativi** (se non vuoi P052): `tex-gyre-fonts` (TeX Gyre Pagella) o `ttf-dejavu` (ripiego). Per forzare un font: `FONT=/percorso/font.otf ./build/sala_segreta`.

### Per la regressione visiva (opzionali, solo sviluppo)
```bash
sudo pacman -S --needed xorg-server-xvfb python python-pillow mesa
```
| Pacchetto | A cosa serve |
|---|---|
| `xorg-server-xvfb` | server X virtuale: fornisce `xvfb-run`, per generare screenshot senza aprire finestre |
| `python`, `python-pillow` | `tools/compare.py` (lettura e confronto delle immagini) |
| `mesa` | renderer software (llvmpipe): rende gli screenshot indipendenti dalla GPU e riproducibili |

### Build alternativa con CMake (opzionale)
```bash
sudo pacman -S --needed cmake ninja
```
`base-devel` include già `make`, quindi per la build normale CMake non serve. Più avanti (passo M11) servirà anche `nlohmann-json` (dati in JSON).

---

## 3. Compilazione

### Metodo consigliato (make, incrementale)
```bash
cd sala_segreta
make -j$(nproc)                # crea build/sala_segreta   (equivalente: bash tools/build.sh)
./build/sala_segreta
make test                      # test headless delle definizioni (nessuna dipendenza da SDL/GL)
make clean                     # cancella build/
```
`make` ricompila solo i file modificati (e quelli che includono le intestazioni toccate). Altra cartella di output: `make BUILD=/percorso`.

### Alternativa: CMake
```bash
cmake -S . -B build -G Ninja   # (senza -G Ninja usa Make)
cmake --build build -j
./build/sala_segreta
```
`CMakeLists.txt` trova da solo SDL2, SDL2_ttf e OpenGL (via `pkg-config` e `find_package`) e raccoglie tutti i `.cpp` di `src/`: aggiungendo un file non serve modificarlo (basta rilanciare `cmake`).

### Cosa fa la build
| Elemento | Significato |
|---|---|
| `-std=c++17 -O2 -Wall -Wextra` | standard richiesto, ottimizzazione, avvisi: il codice attuale compila **senza avvisi** |
| `-Isrc` | cartella delle intestazioni dei moduli (`#include "core/types.h"`, `"render/render.h"`) |
| `sdl2-config --cflags --libs` | opzioni di SDL2 |
| `-lSDL2_ttf -lGL` | libreria dei font e OpenGL |
| `-MMD -MP` (solo Makefile) | genera le dipendenze delle intestazioni per la ricompilazione incrementale |

Tempi indicativi (misurati, un solo core): compilazione completa ≈ 11 s; modifica a un file di `src/render/` o `src/ui/` ≈ 0,5–1,2 s; a `sala_segreta.cpp` ≈ 3,3 s; a un'intestazione condivisa (`items.h`, `state.h`) ≈ 10 s.

### Comando manuale (senza make né CMake)
```bash
g++ -std=c++17 -O2 -Wall -Wextra -Isrc sala_segreta.cpp $(find src -name '*.cpp') \
    -o sala_segreta $(sdl2-config --cflags --libs) -lSDL2_ttf -lGL
```

### Verifica dell'ambiente
```bash
g++ --version                       # >= 9 (C++17)
sdl2-config --version               # SDL 2.x
pkg-config --modversion SDL2_ttf    # SDL2_ttf 2.x
fc-match "P052"                     # deve rispondere con il font P052 (urw-base35-fonts)
glxinfo -B | head                   # (pacchetto mesa-utils) mostra driver e versione OpenGL
```
Se compare un font diverso da P052 il gioco ripiega su TeX Gyre Pagella o DejaVu Serif e lo scrive su stderr all'avvio (`Font: …`).

---

## 4. Avvio e comandi

Finestra 1280×720, mouse catturato (Tab lo libera, Esc esce).
| Tasto | Azione |
|---|---|
| WASD · mouse · Shift | muoversi · guardare · correre |
| E (breve / tenuto) | interagire / trasportare l'oggetto (rilascio = cade) |
| Maiusc+E · F (tenuto) | lasciar cadere ciò che hai in mano · avvicinare le due torce |
| I | inventario (clic = mano destra, clic destro = sinistra, Ctrl+clic = indossa, trascina la figura per ruotarla) |
| SPAZIO | lancio dei dadi all'inizio della partita |
| R | ricomincia dopo la morte |

Il quadro completo dei comandi e delle regole è in `changelog.md` ("Stato attuale") e `rules.md`.

### Opzioni da riga di comando (sviluppo)
```bash
./build/sala_segreta --shot out.ppm [x z yaw° pitch°] [opzioni]   # salva uno screenshot ed esce (deterministico)
```
Opzioni: `--skipdice --demo --inv --fig <°> --dicetime <s> --carry --menu --fire --burn --burnt --open --worn --rackburn --rackburnt --poison <s> --hp <n> --zap --presse --dead --deterministic`.
Esempio: `./build/sala_segreta --shot prova.ppm 2.8 2.2 90 -15 --skipdice --fire`.

---

## 5. Regressione visiva (prima e dopo ogni passo di migrazione)
```bash
bash tools/regress.sh baseline     # dal codice che funziona: compila e salva 19 screenshot in tests/baseline/
# … si modifica il codice …
bash tools/regress.sh check        # compila, genera tests/current/ e confronta: attesi "19/19 scenari uguali"
```
Per tollerare differenze minime di rendering: `bash tools/regress.sh check --tol 2`. In caso di differenze `compare.py` scrive `tests/current/diff_<scenario>.png`.
I riferimenti dipendono dalla versione di Mesa: vanno generati **sulla tua macchina**, non condivisi.

---

## 6. Problemi comuni

| Sintomo | Causa e rimedio |
|---|---|
| `fatal error: SDL2/SDL_ttf.h: No such file` | manca `sdl2_ttf` → `sudo pacman -S sdl2_ttf` |
| `fatal error: GL/gl.h: No such file` / `cannot find -lGL` | mancano intestazioni/libreria OpenGL → `sudo pacman -S libglvnd` (e il driver della GPU) |
| `make: sdl2-config: No such file` (o `Could NOT find SDL2` in CMake) | manca `sdl2` o `pkgconf` → `sudo pacman -S sdl2 pkgconf` |
| `undefined reference to …` dopo aver aggiunto un file | con make basta rilanciare; con CMake rilancia `cmake -S . -B build` per ricaricare l'elenco dei `.cpp` |
| Errori strani dopo aver cambiato un'intestazione | `make clean && make -j$(nproc)` |
| `sdl2-config: command not found` | manca `sdl2` (o il pacchetto di compatibilità) → `sudo pacman -S sdl2`; poi `sdl2-config --version` |
| `Nessun font trovato: i messaggi andranno su terminale` | installa `urw-base35-fonts` oppure indica un font: `FONT=/usr/share/fonts/TTF/DejaVuSerif.ttf ./build/sala_segreta` |
| Il testo mostra "œ" al posto di "ce" | già gestito nel codice; se compare, segnalarlo (font diverso da P052) |
| Finestra nera o `Contesto OpenGL: …` | driver OpenGL non attivo: controlla `glxinfo -B`; prova `LIBGL_ALWAYS_SOFTWARE=1 ./build/sala_segreta` (lento ma funziona) |
| `xvfb-run: command not found` | `sudo pacman -S xorg-server-xvfb` (solo per `tools/shots.sh`) |
| `ModuleNotFoundError: PIL` | `sudo pacman -S python-pillow` (solo per `tools/compare.py`) |
| `Permission denied` su `tools/*.sh` | lancia con `bash tools/...` oppure `chmod +x tools/*.sh` |
| Il mouse resta catturato | premi **Tab**; **Esc** chiude il gioco |

---

## 7. Note di piattaforma
- Testato in ambiente **Linux x86-64 / X11** (anche con Xvfb per gli screenshot). Su Wayland SDL2 usa XWayland o il backend nativo: il gioco non richiede nulla di specifico.
- Nessuna rete e nessuna telemetria: il gioco lavora solo in locale.
- I salvataggi non esistono ancora (passi M10 e F8): per ora ogni avvio parte da zero.
