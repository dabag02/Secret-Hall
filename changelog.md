# Changelog — La Sala Segreta

Dungeon 3D in wireframe, prima persona, C++17 + SDL2 + SDL2_ttf + OpenGL a pipeline fissa.
Questo file è la memoria del progetto: **leggilo per primo in una nuova chat**. Va aggiornato a ogni sviluppo
(nuova voce in cima a "Cronologia", poi aggiornare "Stato attuale" se cambia).

---

## Stato attuale (aggiornato: v0.7.8 — M4c completato)

**Sorgenti:** `sala_segreta.cpp` (954 righe: logica di interazione, dadi, `main`) + `src/` (core, gfx, **game**, render, ui, items, stato condiviso, fisica dello scheletro: 31 file) + `tests/unit/test_defs.cpp` (`make test`). Build incrementale con `make` o CMake.
**Documenti di progetto:** `rules.md` v2 (regole con ID A1…Q2: generali + addendum roguelike, stato di ciascuna, specifiche e decisioni aperte), `architecture.md` v3 (architettura e roadmap M0–M12 + F1–F20).
**Migrazione modulare:** in corso — **M0**–**M3** completati; **M4 è diviso in sotto-passi:** **M4a ✅** (tabelle `ObjectDef`/`MaterialDef` in `src/game/defs.*`, via 6 `switch`) · **M4b ✅** (`Entity` unica al posto di `Item`/`Loose`) · **M4c** in due tranche: **1 ✅** scrigno e secchio con contenuto come entità (via `g_chestOpen/Gem/Coins`, `g_lid`, `g_keyInBucket`), **2 ✅** arredi e appesi: tavolo, sedia, scheletro, rastrelliera, ganci di spada e scudo (via `g_rackHas`, `g_swordOnWall`, `g_shieldOnWall`, `g_tableState`, `g_rackState`, `g_skelFree`) → **M4c completato** · M4d (`Actor`) · M4e (bersagli senza `HOT[]`/`kind`). Prossimo passo: **M4d** (il giocatore come `Actor`).
**Struttura attuale** (albero completo e dipendenze Arch in `INSTALL.md`): `sala_segreta.cpp` · `Makefile` · `CMakeLists.txt` · `INSTALL.md` · `changelog.md` · `rules.md` · `architecture.md` · `src/` (`core/`, `gfx/`, `render/`, `ui/`, `items.h/.cpp`, `state.h/.cpp`, `skeleton_physics.h/.cpp`) · `tools/` (`build.sh`, `shots.sh`, `compare.py`, `regress.sh`) · `archive/` (`sala_segreta_v0.7.cpp`) · `tests/` (riferimenti locali, non condivisi). **Ogni passo di migrazione si consegna anche come archivio completo** (`sala_segreta_evo_<passo>.tar.gz`/`.zip`).

**Compilazione (Arch Linux):**
```bash
sudo pacman -S --needed sdl2 sdl2_ttf mesa urw-base35-fonts
make -j$(nproc)                    # build incrementale in build/sala_segreta   (equivalente: bash tools/build.sh)
./build/sala_segreta
# alternativa: cmake -S . -B build -G Ninja && cmake --build build   (dipendenze: vedi INSTALL.md)
```

**Comandi di gioco:** WASD muovi · mouse guarda · Shift corri · E interagisci (tenuto premuto: trasporta, rilascio: cade) ·
Maiusc+E lascia cadere ciò che hai in mano · I inventario · F (tenuto) avvicina le due torce · Tab libera il mouse · Esc esci/chiudi ·
Inventario: clic = mano destra, clic destro / Maiusc+1–9 = mano sinistra, Ctrl+clic = indossa armatura, clic su slot di equipaggiamento = indossa/togli,
trascina la figura (o frecce) = ruota · SPAZIO = lancia i dadi (minigioco iniziale) · R = riparti dopo la morte.

**Opzioni di test (`--shot file.ppm [x z yaw° pitch°]` più):** `--deterministic` (implicito con `--shot`) `--presse` (preme E sull'oggetto puntato) `--dead` `--skipdice --demo --inv --fig <°> --dicetime <s> --carry --menu --fire --burn --burnt --open --worn --rackburn --rackburnt --poison <s> --hp <n> --zap`.
**Regressione (da usare a ogni passo di migrazione):** `bash tools/regress.sh baseline` prima del passo (dal codice che funziona), `bash tools/regress.sh check` dopo; richiede `xvfb-run` e `python-pillow` (`sudo pacman -S xorg-server-xvfb python-pillow`). I riferimenti si generano **in locale** (dipendono dalla versione di Mesa).

### Mappa del sorgente (cosa sta dove, per orientarsi)
| Posizione | Contenuto |
|---|---|
| `src/core/` | `types.h` (`PI`, `V`, operazioni vettoriali), `config.h` (costanti: sala 10×4 m, raggio e limiti del giocatore, 20 kg, torce, peso dello scheletro) |
| `src/gfx/primitives.*` | Primitive wireframe: `PM`, `col` (con tinta "rovente"), `line` (spezzata ogni 0,5 m per l'illuminazione per vertice), `box`, `cone`, `dome`, `arc`… |
| `src/state.h/.cpp` (arredi: `g_fixtures`, `fixture()`, `hang/unhang/spillSupport`) | Stato **condiviso tra logica e disegno** (interim fino a M5): pareti `W[4]`, torce a muro, oggetti sciolti `Loose`, stato di camino/tavolo/rastrelliera/scrigno, `g_time` |
| `src/game/entity.h` | **(M4b, M4c)** `Entity`: l'unico tipo di oggetto (mani, inventario, a terra, trasportato, arredi); `contents` = oggetti contenuti o appesi (con `slot`); stato di fuoco, coperchio e braccia; `holds()`, `contentIndex()` |
| `src/game/defs.*` | **(M4a)** `DefId` (D_KEY…D_ASH, i valori 1–15 coincidono con i vecchi `Item::type`), `ObjectDef` (nome, articolo, massa, materiale, flag, `visualId`), `MaterialDef`; `defOf()`, `materialOf()`, `defHas()`. Nessuna dipendenza da GL/SDL |
| `src/items.h/.cpp` | inventario e mani (`g_inv`, `g_hand`), equipaggiamento (`g_equip`), statistiche (`g_stats`, `g_hp`), pesi (`itemKg`, `looseKg`, `totalKg`…), bonus dell'armatura (`effStat`, `hpMax`), nomi degli oggetti |
| `src/skeleton_physics.*` | Fisica dello scheletro: posa delle braccia (IK sul tavolo / doppio pendolo libero) |
| `src/render/` | `render.h` (elenco delle funzioni di disegno), `room`, `wall_props`, `models` (+ `drawItemModel`), `rack`, `furniture`, `skeleton`, `scene` (`drawWorld`), `lighting` (luci e camera), `viewmodel` (oggetti in mano) |
| `src/ui/` | `text.*` (`TextCache`, riquadri), `draw2d.*` (`fillRect`/`strokeRect`), `figure.*` (uomo wireframe), `inventory_ui.*` (pannello inventario, icone), `hourglass.*` (`Countdown` + clessidra) |
| `sala_segreta.cpp` 66–139 | Oggetti sciolti e inventario (logica): `hasKey`, `handWith`, `giveItem`, `weightFail`, nomi, `torchAxisPos` |
| 140–255 | Interazione, collisioni ed equipaggiamento: hotspot, `interact()`, `blocked`, `validDrop`, `slotAction`, `autoEquip`, `equipClick` |
| 256–344 | Minigioco dei dadi (logica e disegno) |
| 345–969 | `main()`: ~11 lambda (azioni) + ciclo (input, simulazione, render, HUD) |

### Regole di gioco già decise (non ridiscutere senza motivo)
- **Statistiche:** 5 caratteristiche (Forza, Destrezza, Magia, Carisma, Vita) = migliore di 3 lanci di 2d6, per ciascuna. Vita è anche il massimo dei punti vita.
- **Armatura:** 10 pezzi (elmo, corazza, 2 spalline, 2 bracciali, 2 gambali, 2 stivali). Bonus Forza/Vita +2, malus Destrezza/Magia −1,
  applicati **una volta sola** se si indossa almeno un pezzo (costante `ARMOR_BONUS_PER_PIECE = false` per cambiare).
- **Peso:** massimo 20 kg totali (inventario + mani + indossato + oggetto trasportato). Pesi in `itemKg()`: chiave 0,15 · torcia 1,2 · spada 1,6 · scudo 3 · secchio 3 ·
  gemma 0,1 · monete 0,9 · libro 1,2 · elmo 1,8 · corazza 5 · spallina 1 · gambale 1,5 · bracciale 0,8 · stivale 1,2 · scrigno 12 (non entra nell'inventario) · scheletro 9 (10,8 con elmo, fisso).
- **E:** pressione breve = interagisci (alcune scelte con menu 1/2); tenuta oltre 0,35 s = trasporta; rilascio = cade davanti ai piedi. Immobili: scheletro, porte, quadri.
- **Fuoco:** una torcia accesa avvicinata ai ceppi accende il camino; una torcia spenta avvicinata al fuoco si accende; due torce in mano si accendono con F.
- **Tavolo/rastrelliera:** una torcia accesa caduta sul tavolo/ai suoi piedi lo incendia per ~4,5 s e poi crolla; ciò che c'è sopra cade. L'armatura caduta dalla rastrelliera è rovente 10 s (−1 vita per tentativo di tocco).
- **Porte magiche:** una torcia accesa vicina a una porta dà una scarica (−1 vita, pausa 2,5 s).
- **Libro:** brucia vicino a una fiamma / nel camino acceso / sul tavolo in fiamme; la cenere è tossica: −1 vita al secondo per 10 s (clessidra in basso a destra). Vita 0 = morte.
- **Fisica:** tutti gli oggetti sciolti hanno peso, gravità e rimbalzo (inversamente al peso). Scheletro: braccia = doppio pendolo che cade quando il tavolo crolla.

### Soluzioni tecniche da ricordare
- Illuminazione = luci GL solo "ambientali" con attenuazione quadratica; calcolata **per vertice**, quindi le linee lunghe sono spezzate a 0,5 m in `line()`.
- Font: il P052 trasforma "ce" in una legatura "œ"; `TextCache::get` inserisce uno spazio sottile U+200A tra "c" ed "e" (U+200B/U+200C non funzionano con `TTF_RenderUTF8_Blended_Wrapped`).
- `--shot` salva un PPM dopo 3 frame ed esce; **dal M0 il tempo è a passo fisso 1/60 s e i dadi hanno seme fisso** (riproducibile bit a bit).
- `restart` dopo la morte usa `execv(argv[0])`.

### Debito tecnico noto
- Un solo file; 35 variabili globali `g_*`; `main()` di ~650 righe con 11 lambda.
- 13 `switch` sul tipo di oggetto da aggiornare a ogni nuovo oggetto.
- Rendering in modalità immediata: ~10.400 vertici e ~1.900 `glBegin` per frame (misurati) — ottimizzabile con display list sulla geometria statica.
- Compilazione ~5,4 s (O2) a ogni modifica.

---

## Cronologia

### v0.7.8 — M4c (tranche 2): arredi e oggetti appesi come entità (B1, B6, F2)
- **Nuovi arredi nei dati (`game/defs`):** `D_TABLE`, `D_CHAIR`, `D_SKELETON`, `D_RACK`, `D_SWORD_HOOKS`, `D_SHIELD_HOOKS` (flag nuovo `F_FIXED`; rastrelliera e ganci sono anche `F_CONTAINER`), materiale nuovo `M_BONE` (osso, 1,9 da `rules.md`). Lo scheletro pesa 9 kg (`SKELETON_KG`). **Le masse di tavolo, sedia, rastrelliera e ganci non sono decise: 0 = ignota** finché F1 non li rende spostabili (non ho inventato valori).
- **`g_fixtures` (`state.h/.cpp`):** elenco delle entità-arredo della sala, creato da `makeFixtures()` (configurazione originale: sarà il punto d'ingresso della nuova Run in M5); accesso con `fixture(D_TABLE)`… e le letture `tableState()`, `rackState()`, `rackHas(i)`, `swordOnWall()`, `shieldOnWall()`.
- **"Appeso" = contenuto del supporto:** i 9 pezzi dell'armatura stanno in `rack.contents` (ognuno con il proprio `slot` 0–8, così le due spalline/gambali/bracciali/stivali restano distinguibili), spada e scudo nel `contents` dei rispettivi ganci. Funzioni testate `hang()`, `unhang()`, `hungIndex()`, `spillSupport()`; il crollo della rastrelliera fa cadere nel mondo tutto ciò che è ancora appeso, rovente (10 s) e nella posizione del suo posto. Gli appesi restano in ordine di posto qualunque sia la storia.
- **Stato del fuoco nell'entità:** `fireState` (0 integro, 1 in fiamme, 2 crollato) e `fireT` sul tavolo e sulla rastrelliera al posto di `g_tableState/g_rackState` e dei timer `g_tableT/g_rackT`; `armsFree` sullo scheletro al posto di `g_skelFree` (usato dalla fisica delle braccia).
- **Spariti 8 globali:** `g_tableState`, `g_rackState`, `g_rackHas[9]`, `g_swordOnWall`, `g_shieldOnWall`, `g_skelFree`, `g_tableT`, `g_rackT` (più il duplicato `RACK_TYPE` statico del `main`, ora in `state.cpp`). Nessun globale "di stato oggetti" rimasto tranne quelli elencati sotto.
- **Verifica (eseguita):** build pulita, **0 avvisi** · `make test` superato (nuove prove: configurazione iniziale, stacco/riappendi, ordine, crollo con un pezzo già preso, ganci) · regressione **19/19 identici** (compresi i 4 scenari che toccano rastrelliera e tavolo: in fiamme, crollati, `--worn`, `--demo`).
- **Non coperto dagli screenshot (da provare a mano):** prendere un pezzo dalla rastrelliera (E breve e E tenuto) e vedere che gli altri restano ai loro posti (provare le due spalline) · prendere e riappendere spada e scudo · incendiare il tavolo e la rastrelliera con una torcia accesa lasciata cadere (crollo e caduta di ciò che c'è sopra/appeso, armatura rovente) · braccia dello scheletro che cadono dopo il crollo del tavolo.
- **Ancora da convertire (non in questa tranche):** le torce a muro `g_wt[4]` (presente/accesa), il fuoco del camino `g_fireLit`, le porte (`g_zapDoor`) e gli stati di sala `W`, `HOT[]`; arriveranno con M4e/M5 (e con F13 per camino e torce). `Entity` ha ancora campi "piatti" (`fireState`, `slot`, `armsFree`, `open`…): diventeranno componenti (`Thermal`, `Anim`, `Mount`, `Container`) con F1/F2.

### v0.7.7 — M4c (tranche 1): scrigno e secchio con contenuto come oggetti veri (B6)
- **`Entity` diventa anche contenitore:** nuovi campi `contents` (elenco di `Entity`), `open` (lucchetto aperto) e `openDeg` (animazione del coperchio, stato visivo); helper `holds()` e `contentIndex()`. **Il contenuto viaggia con il contenitore**: un secchio con la chiave tenuto in mano, nell'inventario, trasportato o lasciato cadere resta "da frugare"; prima lo stato era un flag globale valido per un solo secchio.
- **Spariti 5 globali:** `g_chestOpen`, `g_chestGem`, `g_chestCoins`, `g_lid`, `g_keyInBucket`. Nei dati iniziali (`state.cpp`) il secchio contiene la chiave di bronzo e lo scrigno gemma e monete (in quest'ordine); sparisce anche il caso speciale interim sul nome del secchio (ora legge il contenuto).
- **Interazione:** i bersagli `kind` 5/6 (gemma/monete) diventano un unico `kind 5` = "contenuto di un contenitore" (`HotD` ha il nuovo campo `idx2`: posizione nel contenuto); frugare il secchio sposta la chiave dal contenuto all'inventario/mani; prelevare dallo scrigno rimuove l'oggetto dal suo `contents`. I messaggi e le etichette sono invariati.
- **Render:** `drawChest(const Entity&)` disegna lucchetto/coperchio/contenuto dallo stato dell'entità (gemma che ruota e monete come prima).
- **Verifica (eseguita):** build pulita, **0 avvisi** · `make test` superato (nuovi controlli: il contenuto viaggia con il secchio, le copie sono indipendenti, scrigno con contenuto) · regressione **19/19 identici**: ai 17 scenari si aggiungono **18** (menu dello scrigno chiuso con la chiave) e **19** (contenuto dello scrigno aperto puntato con E), con baseline generata dal codice M4b prima della modifica.
- **Non coperto dagli screenshot (da provare a mano):** frugare il secchio (da terra, dall'inventario, dalla mano) e ricevere la chiave · aprire lo scrigno con la chiave · prendere gemma e monete (inventario, mano, E tenuto) · portare/lasciare lo scrigno aperto · lasciare cadere il secchio dopo averlo frugato. La logica è nelle lambda del `main`, non ancora testabile in modo headless (M6/M12).
- **Invariato di proposito:** il peso del contenuto non conta nel carico (la chiave nel secchio non pesa); la regola N1 (peso del contenuto dello scrigno sommato al trasporto, 6 slot, riporre) arriva con F15.

### v0.7.6 — M4b: un solo concetto di oggetto (`Entity`)
- **Nuovo `src/game/entity.h`:** `struct Entity {def, lit, pos, yaw, vy, hot, burn}` sostituisce **sia `Item` (inventario, mani) sia `Loose` (oggetti a terra, oggetto trasportato)**. `def` è un `DefId`; in una mano o in uno slot contano solo `def` e `lit`, il resto vale per gli oggetti nel mondo.
- **Eliminata la seconda numerazione:** spariti `LT_*`, il campo `Loose::sub` (i pezzi d'armatura sono ora direttamente `D_HELM`…`D_BOOT`), `ObjectDef::looseType` e le conversioni `itemOfLoose`, `looseOfItem`, `defOfLoose`, `looseName`, `looseKg` (si usano `defArticle(e.def)` e `itemKg(e.def)`). Lasciar cadere un oggetto è ora una copia dell'`Entity` con la posizione; raccogliere passa da `slotOf()` (tiene solo `def`/`lit`, `D_NONE` per scrigno e ceneri, che non sono inventariabili).
- **Switch di disegno e di menu** (`drawLooseObj`, etichette dei bersagli, `shortLoose`) ora ragionano su `D_*`; i modelli e i testi sono invariati. `defs.cpp` non include più `state.h`.
- **Test (`make test`):** le copie `legacy::` dei vecchi `switch` sono sostituite da una tabella di valori attesi (nome, nome da acceso, articolo, peso, inventariabilità per tutti i 16 oggetti) più controlli su `slotOf` e sul peso totale (inventario + mani + indossato + oggetto trasportato).
- **Verifica (eseguita):** build pulita, **0 avvisi** · `make test` superato · regressione visiva **17/17 identici** alla baseline M3.
- **Interim rimasti:** il nome del secchio dipende ancora da `g_keyInBucket`; `Entity` ha ancora campi fissi invece di componenti (M4c); le liste `g_loose`, `g_hand`, `g_inv`, `g_carry` restano globali (M4d/M5).
- **Consegna:** da questo passo si consegnano solo i file creati o modificati (archivio completo riservato alle milestone).

### v0.7.5 — M4a: definizioni di oggetti e materiali (`src/game/defs.*`)
- **Nuovo `src/game/defs.h/.cpp`** (livello `game/`, senza GL/SDL): `enum DefId` (D_KEY, D_TORCH, D_SWORD, D_SHIELD, D_BUCKET, D_GEM, D_COINS, D_BOOK, D_HELM…D_BOOT, D_CHEST, D_ASH), `ObjectDef` {chiave, nome, nome da acceso, nome con articolo, massa, materiale, flag `F_CARRIABLE/INVENTORY/WEARABLE/CONTAINER/LIGHTABLE`, tipo sciolto interim, `visualId`} e `MaterialDef` {densità, infiammabile, livello di fusione}. Pesi e nomi **identici** a v0.7 (B1). Materiali con densità da `rules.md`; **provvisori** (da confermare): densità di gemma (3,5) e cenere (0,5), materiale "acciaio" per i pezzi d'armatura. Oggi tabelle C++, in M11 diventeranno `objects.json`/`materials.json`.
- **6 `switch` sul tipo di oggetto eliminati** (erano 18 in tutto il codice, ora 12; gli altri sono dispatch di disegno e menu, restano ai passi M4e/M7/M8): `itemName`, `itemKg`, `itemOfLoose` (in `items.cpp`) e `armorArt`, `looseName`, `looseOfItem` (erano `static` in `sala_segreta.cpp`; ora in `items.cpp` e dichiarati in `items.h`, con `defArticle`, `defOfLoose`).
- **Numeri magici sostituiti dai nomi `D_*`** in `sala_segreta.cpp`, `render/models`, `render/viewmodel`, `ui/inventory_ui`, `SLOT_TYPE`, `RACK_TYPE`; `isArmor()` ora legge il flag `F_WEARABLE`. Nessun cambio di valori.
- **Primo test headless (J1 in anticipo):** `make test` (o `cmake --build build --target test_defs`) → `tests/unit/test_defs.cpp`, senza SDL/GL. Confronta le tabelle con **copie verbatim dei vecchi `switch`** su tutti i tipi, verifica i pesi di riferimento (armatura completa 15,8 kg, scrigno 12 kg…), la coerenza della tabella (chiavi uniche, materiale presente, indossabile ⇒ inventariabile). Le copie `legacy::` si cancellano dopo M4b. Il test ha scoperto subito un errore di indicizzazione della tabella durante lo sviluppo.
- **Differenze volute rispetto a prima, non raggiungibili in gioco:** `itemKg(D_CHEST)` ora dà 12 (prima 0, ma uno scrigno non è mai un `Item`); `looseOfItem` di un tipo 20/21 dà scrigno/cenere (prima ripiegava sull'armatura).
- **Correzione di compilazione (SDL_ttf recente, Arch):** `text.h` dichiarava in anticipo `struct _TTF_Font`, ma le versioni nuove di SDL_ttf chiamano la struct `TTF_Font` → errore in `text.cpp`. Ora `TextCache::font` è un `void*` opaco con cast in `text.cpp`: compila con entrambe le convenzioni (verificato con un header simulato). Origine: introdotto in M3.
- **Interim rimasti:** il nome del secchio dipende ancora da `g_keyInBucket` (caso speciale in `itemName`, sparisce in M4c); `defs.cpp` include `state.h` per i `LT_*` (sparisce in M4b).
- **Verifica (eseguita):** build pulita con **0 avvisi** · `make test` superato · regressione visiva **17/17 identici** alla baseline generata dal codice M3 nella stessa sessione (Xvfb + Mesa software). Il monolite scende da 969 a **954 righe**.

### v0.7.4 — M3: interfaccia in `src/ui/`
- **Spostato in `src/ui/` (codice tagliato com'era, con script, non riscritto):** testo (`TextCache`, ricerca del font, riquadri) in `text.*`; `fillRect`/`strokeRect` in `draw2d.*`; uomo wireframe (`limb`, `drawMan`) in `figure.*`; pannello dell'inventario (`InvLayout`, `invLayout`, `slotRect`, `invHit`, `figHit`, `drawItemIcon`, `drawInventory`) in `inventory_ui.*`; clessidra in `hourglass.*`.
- **`TextCache` ora ha un'interfaccia pulita:** dichiarazione in `text.h` (senza includere SDL_ttf: `struct _TTF_Font` dichiarata in anticipo), implementazione in `text.cpp`.
- **Clessidra generica dei countdown (regola P1):** `struct Countdown {etichetta, residuo, totale}` e `drawCountdowns()` che disegna una clessidra per ciascun conto alla rovescia, impilate da destra verso sinistra. Per ora c'è solo il veleno; il resto (buio, oggetti roventi, torcia in esaurimento) si aggiunge con una riga. Posizione e aspetto sono identici a prima.
- **`src/items.h/.cpp` (nuovo, interim fino a M4–M5):** `Item`, inventario, mani, equipaggiamento, statistiche, punti vita, pesi, bonus dell'armatura e nomi degli oggetti: tutto ciò che logica e interfaccia devono condividere. Le funzioni che *modificano* lo stato (`giveItem`, `slotAction`, `equipClick`…) restano nel file principale.
- **`src/render/viewmodel.cpp` (nuovo):** `drawHandItem` (oggetto in mano in prima persona); `drawItemModel` si aggiunge a `render/models.cpp`.
- **Verifica:** i 17 scenari sono **17/17 identici** a M0; build pulita con **0 avvisi**.
- **Compilazione incrementale (misurata, 1 core):** un file di `ui/` o `render/` ≈ 0,5–1,2 s; `sala_segreta.cpp` ≈ 3,3 s (era ≈ 4 s); build completa ≈ 11 s.
- Il monolite scende da 1371 a **969 righe** (da 2142 all'origine, −55%).

### v0.7.3 — M2: build multi-file e modulo render/
- **Build:** `Makefile` (incrementale, con dipendenze `-MMD`) e `CMakeLists.txt` (Ninja/Make) per compilare `sala_segreta.cpp` + tutti i `.cpp` di `src/`; `tools/build.sh` ora usa `make`. Entrambi producono lo stesso eseguibile (stessa dimensione) e **0 avvisi**.
- **Spostato in `src/render/` (senza riscrivere il codice, con uno script che taglia le funzioni e ne rigenera le dichiarazioni):** stanza (`room`), oggetti a muro (`wall_props`), modelli piccoli (`models`), rastrelliera (`rack`), arredi (`furniture`), scheletro (`skeleton`), scena (`scene`: `drawLooseObj`, `drawWorld`), illuminazione e camera (`lighting`: `setLights`, `lookAt`). Dichiarazioni in `render/render.h`.
- **Stato condiviso interim (`src/state.h/.cpp`):** i tipi `Wall`, `Hole`, `WallTorch`, `Loose` + enum `LT_*` e le variabili usate anche dal disegno (`W`, `DOOR_U`, `TORCH_U`, `g_wt`, `g_loose`, `g_carry`, `g_fireLit`, `g_tableState`, `g_rackState`, `g_rackHas`, `g_chestOpen/Gem/Coins`, `g_lid`, `g_time`, …) sono definite una volta in `state.cpp` e dichiarate `extern`. Le altre variabili (inventario, mani, vita…) restano nel file principale. **Tutto questo sparirà in M5** (`GameState`).
- **`src/skeleton_physics.*`:** `armPose` e `updateSkeleton` (logica pura) separati dal disegno dello scheletro; `SKELETON_KG` passa in `core/config.h`.
- **Nessun cambiamento di comportamento:** i 17 scenari sono **17/17 identici** a M0 sia con `make` sia con CMake.
- **Compilazione incrementale (misurata):** modifica a un file di `render/` → ~0,6 s; a `sala_segreta.cpp` → ~4 s; a `state.h` (incluso quasi ovunque) → ~9 s (con `-j4`); prima: ~5 s per qualunque modifica, con un solo file.
- Il monolite scende da 2051 a 1371 righe (da 2142 all'origine).
- **Consegna:** per un errore mio, i file nuovi di `src/render/`, `skeleton_physics.*` e la versione aggiornata di `tools/build.sh` e `src/core/config.h` non erano stati allegati (build fallita con `skeleton_physics.h: File o directory non esistente`). **Da ora ogni passo include l'archivio completo del progetto** (`sala_segreta_evo_<passo>.tar.gz`/`.zip`), verificato estraendolo in una cartella pulita e compilando.

### v0.7.2 — M1: estratte le primitive (core/ e gfx/)
- **Nuovi file:** `src/core/types.h` (`PI`, `V`, `vadd/vmul/vcross/vnorm`), `src/core/config.h` (costanti condivise: `WL`, `WH`, `PR`, `LIM`, `MAX_KG`, `TORCH_Y/Z/TILT`), `src/gfx/primitives.h/.cpp` (`PM`, `col`, `line`, `strip`, `arc`, `ring*`, `rect`, `box`, `boxAt`, `cone`, `dome`, `sphere`, `coneAlongX`).
- **Cosa è stato spostato (senza riscriverlo):** il blocco "Primitive wireframe" del monolite e le definizioni di `V`/`PI`; le funzioni non sono più `static` ma dichiarate in `primitives.h` con gli stessi nomi e argomenti predefiniti, quindi il resto del codice non è cambiato. Lo stato di tinta (`g_tintHot`, `g_hotPulse`) vive ora in `primitives.cpp` (è stato di *rendering*).
- **Regola dei livelli rispettata:** `core/` non include OpenGL né SDL; `gfx/` include OpenGL.
- **Verifica:** i 17 scenari dopo M1 sono **17/17 identici** a quelli di prima (M0). Nessun avviso del compilatore.
- Il monolite scende da 2142 a 2051 righe; la compilazione di `sala_segreta.cpp` resta ~5 s (la sua dimensione è ancora quella del gioco), `primitives.cpp` si compila in 0,35 s da solo.
- `tools/build.sh` ora compila `sala_segreta.cpp` + tutti i `.cpp` in `src/`.

### v0.7.1 — M0: rete di sicurezza della migrazione
- **Tempo deterministico:** `--deterministic` (passo fisso 1/60 s, seme fisso dei dadi), implicito con `--shot`. Prima gli screenshot dipendevano dal tempo reale (fiamme).
- **`--shot` non preme più E in automatico:** ora serve `--presse`. Nuova opzione `--dead` per provare la schermata di morte.
- **Strumenti** (`tools/`): `build.sh`, `shots.sh` (19 scenari: stanza, dadi, inventario, armatura, mani, camino, tavolo in fiamme/bruciato, scrigno, rastrelliera, menu, trasporto, porta magica, morte, interazione), `compare.py` (confronto pixel per pixel con immagine delle differenze), `regress.sh` (`baseline`/`check`).
- **Verifica:** due esecuzioni consecutive dei 17 scenari → **17/17 identici**.
- Copia di sicurezza del codice v0.7 in `archive/sala_segreta_v0.7.cpp`.
- Regola J2 realizzata. Nessun cambiamento al gioco.
- Nota: gli script vanno lanciati con `bash tools/...` (oppure `chmod +x tools/*.sh`).

### [Non rilasciato] — Contenuto della sala (addendum 3, solo documentazione)
- **Nuove regole R1–R6** (`rules.md` sez. R): spazio nascosto (nicchia) dietro ogni quadro; in una nicchia **attrezzatura alchemica + crogiolo**, in un'altra **pane + sacco di farina** (farina + acqua → impasto → pane cotto); **laghetto dietro la porta nord** con acqua illimitata (scoperto con un enigma); **muschio** sulla parete nord (ingrediente per ricette/incantesimi); **un enigma per ogni quadro e ogni porta** (8) con relativo loot; **una sola porta è l'uscita**.
- `architecture.md` → **v3.1**: sezione 3d "La sala come dati" (mattoni `Concealed`, `WaterSource`, `Harvestable`, `Station`, `LootTable`, `PuzzleSlot`; esempio di `SceneDef`); nuovo passo **F21**; **F10** ridefinito come framework a 8 slot; ordine consigliato e prototipo 1.0 aggiornati.
- Assegnazione provvisoria (nei dati): nicchia sud = alchimia + crogiolo; nicchia est = pane + farina; nord e ovest da definire; porta nord = laghetto; uscita provvisoria = porta ovest.
- Punto aperto: i quadri stanno a 2,95–3,88 m, oltre la salita massima di 1,0 m → predefinito: l'enigma risolto apre il quadro e il loot scende.

### [Non rilasciato] — Addendum roguelike (solo documentazione)
- **Nuove regole** (`rules.md` K–Q): il gioco è un **roguelike** — alla morte la sala torna alla configurazione originale e si rinasce con nuove statistiche (nuovo lancio dei dadi); lo scheletro **eredita le statistiche del morto** (alla prima partita quelle scartate dai dadi); il libro conserva nome, note e cronologia (la cronologia non si cancella); **vittoria** solo aprendo la giusta porta di uscita; **game over definitivo** (si riparte davvero da zero) solo se nessun fuoco illumina più la sala.
- **Sopravvivenza:** sete e fame; **torce e camino si consumano**; oggetti non inceneribili nel fuoco diventano **roventi**; lo **scrigno è un secondo inventario** (6 slot, limiti di dimensione).
- **Attori:** giocatore e scheletro sono attori con le stesse capacità; se il **libro brucia** lo scheletro si risveglia e attacca (e da quel momento non si annota più nulla, nemmeno per la vita successiva). **Move set** (mouse sx/dx breve e tenuto, scudo, Maiusc+W, Ctrl), **combattimento in tempo reale** con confronto delle statistiche in background; lo scheletro non eredita crafting né incantesimi.
- **Clessidra** = simbolo globale di ogni countdown. **Predisposizione online:** modello d'istanza condivisibile (libro + ultimo scheletro), opt-in, nessuna rete oggi.
- **Decisione precedente superata:** "libro bruciato = cronologia conservata ma lettura sospesa" → ora vale O3 (da quel momento non si annota più).
- `architecture.md` → **v3**: il giocatore e lo scheletro diventano `Actor` con `Controller` e comandi comuni; stato a tre livelli **Meta / Run / Istanza** con `SceneDef` (configurazione originale della sala); nuovi sistemi (`needs`, `fuel_light`, `combat`, `movement`, `lifecycle`, `ai`); `ActorPose` e `UiModel.timers` nel contratto logica/grafica; roadmap estesa a **F1–F20** con nuovo ordine consigliato (primo traguardo giocabile: M0–M11 + F1–F3, F7, F8, F11–F15, F10).
- Valori iniziali proposti (da tarare): sete 15 min, fame 30 min, torcia 12 min, ceppi 4 min, buio 15 s, roventi 20 s + 4 s/kg, scrigno 6 slot/classe ≤3; formule di combattimento in `rules.md`.
- Nessuna modifica al codice di gioco (resta v0.7).

### [Non rilasciato] — Regole generali e architettura v2 (solo documentazione)
- **Nuove regole generali** (vedi `rules.md`): ogni oggetto ha peso e proprietà fisiche e reagisce a fuoco/acqua/gravità/lancio (B1–C3); tavolo, scrigno, secchio, sedia e scheletro spostabili e lanciabili (B3, F2); macerie e ceneri sono oggetti con peso (B4); fusione dei metalli e crafting (G1, G2); lancio con **T** con traiettoria prevista, dipendente da peso e Forza, con soglie dei dadi (D2, E2); scheletro con parametri nascosti dai punteggi più bassi dei dadi (F1); libro per nome, note e cronologia delle scoperte (H1, H2, E3); menu con salvataggio (I1, I2).
- **Regola cardine di architettura (A1–A4):** logica e grafica separate da un contratto (`RenderScene`/`UiModel`/`InputState`/`IRenderBackend`) per poter sostituire la grafica (mesh, texture, shader, luci) senza toccare gameplay ed enigmi, e viceversa.
- `architecture.md` riscritto (v2): modello a entità con componenti e definizioni dati (oggetti, materiali, reazioni, ricette), moduli e livelli, moddabilità, roadmap rivista in due parti (M0–M12 architettura, F1–F10 funzioni). Il confine logica/grafica (M7) passa prima delle nuove funzioni.
- Nessuna modifica al codice di gioco (`sala_segreta.cpp` resta v0.7).
- Decisioni aperte con valore predefinito: JSON per i dati; acqua senza fonte finché non c'è una stanza adatta; libro bruciato = cronologia conservata ma lettura sospesa; attrezzi di fusione da aggiungere alla scena; carico massimo fisso a 20 kg.

### v0.7 — Fisica, pesi, salute
- Scheletro: braccia accorciate (omero 30 cm, avambraccio 25 cm), peso 9 kg; al crollo del tavolo le braccia cadono ai fianchi (doppio pendolo smorzato).
- Maiusc+E lascia cadere l'oggetto in mano (anche nel focolare).
- Rastrelliera che brucia e crolla; pezzi d'armatura roventi per 10 s.
- Porte magiche con scarica (−1 vita) e lampo azzurro.
- Icona delle monete ridimensionata. Pesi per tutti gli oggetti, gravità e rimbalzo per tutti gli oggetti sciolti.
- Peso massimo 20 kg con righe del peso nell'inventario; peso di armatura e mani riduce la capacità.
- Libro aperto sul tavolo: prendibile, infiammabile; cenere tossica; veleno con clessidra animata; salute corrente/massima; morte e riavvio con R.

### v0.6 — Fuoco, scrigno, armatura
- Fuoco nel camino acceso da torce (e torce accese dal fuoco). Tavolo che brucia e crolla (lo scrigno cade).
- Scrigno trasportabile (E prolungato, non entra nell'inventario); menu a scelta con la chiave; contenuto (gemma, monete) prelevabile, trasportabile o in mano.
- Elmo e 9 pezzi della rastrelliera raccoglibili e indossabili (10 slot attorno alla figura); bonus/malus alle statistiche.

### v0.5 — Secchio, trasporto, statistiche, dadi
- Secchio raccoglibile; menu a scelta multipla (fruga / tieni nell'inventario); E prolungato trasporta qualunque oggetto mobile, rilascio = cade.
- Statistiche Forza/Destrezza/Magia/Carisma/Vita sotto la figura.
- Minigioco dei dadi iniziale (2d6 sul tavolo, inquadratura ravvicinata, 3 lanci per caratteristica, vale il migliore).
- Correzione della legatura "ce"→"œ" del font.

### v0.4 — Due mani, figura, spada e scudo
- Chiave invisibile finché è sotto la cenere. Due mani (sinistra/destra) con viewmodel. Figura umana wireframe ruotabile (tunica con cintura, calzamaglia, sandali).
- Spada e scudo prelevabili e riappendibili, tenibili in mano entrambi.

### v0.3 — Inventario e torce
- Inventario a 9 slot (tasto I) con mano. Quattro torce a muro (tre accese, una spenta), illuminazione per vertice con fiamme animate.
- Torce prelevabili, in mano o nell'inventario; accensione per contatto (anche con F tra le due mani).

### v0.2 — Testo a schermo
- Riquadro in basso con messaggi (font serif tipo Book Antiqua/Palatino, corpo 14, sfondo semitrasparente) via SDL_ttf.

### v0.1 — Sala wireframe navigabile
- Stanza di pietra 10×10×4 m con 4 porte e ritratti, tavolo con scrigno e lucchetto, scheletro con elmo seduto, camino con spada, scudo e rastrelliera con armatura, secchio di cenere con chiave di bronzo. Movimento in prima persona con collisioni.
