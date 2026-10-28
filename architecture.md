# Architettura e roadmap — La Sala Segreta (v3.1)

Si legge insieme a `rules.md` (requisiti con ID A1…J2) e `changelog.md` (stato e storia).
**v3** integra anche l'*addendum roguelike* (`rules.md` sezioni K–Q: ciclo di vita e morte, sopravvivenza, combustibile, scrigno-inventario, attori e combattimento, online). **v2** integrava le regole generali: oggetti con proprietà fisiche e reazioni, lancio, crafting e fusione, scheletro, diario, salvataggio e — soprattutto — **separazione totale tra logica e grafica** per renderlo modificabile.

---

## 1. Analisi del codice attuale (`sala_segreta.cpp`)

| Metrica | Valore |
|---|---|
| Righe / unità di compilazione | 2142 / 1 (compilazione 5,4 s a ogni modifica) |
| Variabili globali `g_*` | 35 (`g_hand` 51 riferimenti, `g_loose` 31, `g_equip` 20, `g_inv` 17) |
| `main()` | 588 righe, 11 lambda |
| `switch` sul tipo di oggetto | 13 (da toccare per ogni nuovo oggetto) |
| Rendering | ~10.400 vertici, ~1.900 `glBegin` per frame (immediate mode) |

**Problemi strutturali:** (1) file unico con ordine di dichiarazione obbligato; (2) stato globale sparso; (3) "oggetto" replicato in `Item`, `Loose` e flag (`g_chestGem`, `g_swordOnWall`…); (4) `main()` mescola input, regole, fisica, rendering, HUD; (5) numeri magici per bersagli e menu; (6) geometria statica rigenerata a ogni frame.

**Conflitti con le nuove regole (importanti):**
- La logica oggi **chiama OpenGL direttamente** (le funzioni `drawX()` leggono i `g_*` e disegnano): viola A1.
- Gli arredi (tavolo, sedia, scheletro, rastrelliera) non sono oggetti ma codice di disegno + flag: viola B1/B3.
- Fuoco, calore e "rovente 10 s" sono timer ad hoc per oggetto: viola B5/C4.
- Lo stato è sparso in 35 variabili: impossibile salvarlo (I2).

**Da conservare:** primitive wireframe e `PM` RAII, modelli come funzioni pure con origine locale, luci e `TextCache` autonomi, `--shot` per i test visivi.

---

## 2. Principio cardine: il contratto logica ↔ grafica (A1–A4)

```
        INPUT (SDL)                                         OUTPUT (schermo)
            │                                                    ▲
            ▼                                                    │
     ┌──────────────┐   RenderScene + UiModel   ┌───────────────────────────┐
     │ GAMEPLAY     │ ────────────────────────► │ BACKEND GRAFICO           │
     │ (game/,      │   (sola lettura)          │ (render/…, ui/…)          │
     │  systems/)   │                           │ • wireframe_gl (default)  │
     │ nessun GL/SDL│ ◄──── InputState ──────── │ • mesh_gl (modello 3D)    │
     └──────────────┘   (eventi astratti)       │ • altri (shader, texture) │
            ▲                                   └───────────────────────────┘
            │ dati
     data/gameplay/*.json                         data/visual/*.json + assets
     (oggetti, materiali, reazioni,               (modelli, texture, shader,
      ricette, enigmi, stanze)                     luci, tema HUD)
```
**Regole:**
1. `game/` e `systems/` **non includono mai** GL/SDL; ricevono `InputState` (tasti/mouse astratti) e producono `RenderScene` + `UiModel`.
2. Il backend **non modifica** lo stato: legge solo `RenderScene`/`UiModel`.
3. I dati di gameplay contengono **ID visivi** (`visualId`), mai colori, mesh o shader. I dati visivi collegano `visualId` → modello/texture/materiale grafico.
4. Se un ID visivo non ha asset nel backend attivo, si usa il **modello wireframe procedurale** predefinito (A3, A5): il gioco resta sempre giocabile.
5. Le **luci** sono dati logici (posizione, intensità, temperatura colore, seme dello sfarfallio); come illuminare lo decide il backend (oggi: luci ambientali per vertice).

```cpp
struct Renderable { EntityId id; VisualId model; Transform xf; VisualState vs; };   // vs: acceso, carbonizzato, caldo 0..1, bagnato, aperto…
struct LightSrc   { V pos; float intensity; V color; float flickerSeed; };
struct RenderScene{ Camera cam; std::vector<Renderable> items; std::vector<LightSrc> lights;
                    std::vector<TrajectoryPoint> aimPreview; std::vector<Effect> effects; };   // effects: fiamme, scarica, fumo…
struct UiModel    { /* testo, slot, barre, menu, diario… come dati, senza disegno */
                    std::vector<Countdown> timers; };   // Countdown{etichetta, residuo, totale}: ogni conto alla rovescia (veleno, buio, oggetti roventi, torcia in esaurimento) = una clessidra (P1)
struct IRenderBackend { virtual bool init(const AssetRegistry&) = 0;
                        virtual void drawScene(const RenderScene&) = 0;
                        virtual void drawUi(const UiModel&) = 0; virtual ~IRenderBackend() = default; };
```
Un modder grafico scrive un nuovo backend (o sostituisce asset) conoscendo **solo** queste strutture.

---

## 3. Modello a oggetti: entità + componenti "leggeri" (B1–B6, C1–C4, F2–F3)

Tutto ciò che sta nella scena è un'**entità** (tavolo, sedia, scheletro, scrigno, secchio, libro, torce, ceneri, macerie, lingotti…). Nessun motore ECS esterno: una struct con componenti opzionali.

```cpp
struct Entity { EntityId id; DefId def; Transform xf; Physical ph; Thermal th; Wet wet;
                Carried carry; Container* cont; std::optional<Stats> stats; AnimFlags anim; /* … */ };
```
| Componente | Contenuto |
|---|---|
| `Physical` | massa (da definizione o derivata dal materiale), dimensioni, velocità, restituzione, attrito, `fixed` |
| `Thermal` | temperatura, in fiamme, tempo di combustione residuo |
| `Wet` | bagnato 0..1 (non brucia finché asciutto) |
| `Container` | elenco di entità contenute, capacità (scrigno, secchio, crogiolo, inventario) |
| `Stats` | caratteristiche (personaggio, scheletro nascosto) |
| `Anim` | animato sì/no (lo scheletro è inerte finché un evento non lo anima) |

**Definizioni (dati):**
- `ObjectDef`: id, nome, articolo ("la spada"), materiale, massa o volume, dimensioni, restituzione, attrito, flag (`carriable`, `wearable`, `container`, `fixed`, `throwable`, `animatable`), slot, `visualId`, prodotti.
- `MaterialDef`: id, densità, infiammabilità (temperatura di accensione, durata, **prodotti**: cenere/carbone/macerie con frazione di massa), fusione (livello di calore, prodotto fuso), fragilità (soglia d'urto), comportamento con l'acqua.
- `ReactionDef`: **agente** (fuoco, acqua, urto, calore) × **materiale** → **effetto** (accende, spegne, bagna, rompe, fonde). Le reazioni sono dati (C4): niente più codice per oggetto.
- `RecipeDef`: ingredienti (materiale e massa o oggetti), stazione/strumento, livello di calore, tempo → risultati, resa.

**Cosa sparisce:** `Item`/`Loose` separati, 13 `switch`, flag come `g_chestGem`, `g_swordOnWall`, `g_rackHas[]` (spada, scudo, pezzi d'armatura e contenuti diventano entità; "appeso" = relazione con un supporto).

---

## 3b. Attori: giocatore e scheletro (O1, O4–O6)

Il giocatore **non è più un insieme di variabili** (`px`, `pz`, `g_hand`, `g_inv`…): è un `Actor`. Lo scheletro è un **secondo Actor** che usa le stesse strutture e gli stessi sistemi: "stesse capacità" significa "stesso codice".
```cpp
struct Actor { EntityId id; Transform xf; float eyeHeight; Stats stats; Health hp; Needs needs;      // sete, fame
               Inventory inv;            // 9 slot, 2 mani, 10 slot di equipaggiamento
               Abilities abil;           // crafting, incantesimi (lo scheletro: canCraft=false, canCast=false)
               MoveSet moves; ActorMode mode; Controller* ctl; };
```
- **Controller:** `PlayerController` (InputState → comandi) e `SkeletonAI` (stato del mondo → comandi). I **comandi** (`Move`, `Crouch`, `Climb`, `Use`, `Pickup`, `Carry`, `Drop`, `Throw`, `Slash`, `Thrust`, `Parry`, `Push`, `Stow`…) sono l'unica interfaccia verso i sistemi, che non sanno chi li ha generati.
- **Stato visivo:** il backend non riceve "lo scheletro" ma un `ActorPose` (`anim`: Seated, Idle, Walk, Crouch, Climb, Slash, Thrust, Guard, Push, Dead; fase 0..1; oggetti in mano; equipaggiamento). Il backend wireframe lo anima con un **rig procedurale**; un mod può usare mesh e animazioni proprie. Anche il viewmodel in prima persona (spada che colpisce, scudo che para) è guidato dallo stesso `ActorPose`.
- **Movimento in 3D:** il giocatore ha quota e altezza degli occhi (accovacciato 1,15 m); le entità hanno forme di collisione (AABB) con flag `climbable` (tavolo, sedia, scale). Maiusc+W sale fino a 1,0 m.
- **Combattimento:** componenti `Weapon{danno, portata, velocità}` e `Shield`; il sistema `combat` confronta in background le statistiche (formule in `rules.md`, dati in `combat.json`) e produce danni, parate, spinte.

## 3c. Stato a tre livelli: Meta, Run, Istanza (K1–K6, I3, Q1–Q2)

```cpp
struct GameState { Meta meta; Run run; Instance* inst; };
```
| Livello | Contiene | Vive |
|---|---|---|
| **Run** | la sala (entità), gli attori, tempo, bisogni, flag di stanza | creata da una `SceneDef` (dato: la **configurazione originale**) alla nascita; **scartata** alla morte |
| **Meta** | numero di vite, abilità note, flag di vittoria, riferimento all'istanza | sopravvive alla morte; cancellato solo dal game over definitivo |
| **Istanza** | **libro** (nome, note, cronologia in sola aggiunta) e **statistiche dello scheletro** | oggi locale; domani condivisibile (`InstanceStore`) |

**Cicli:** *nascita* = `Run::spawn(SceneDef, istanza)` + minigioco dei dadi · *morte* = `istanza.scheletro = statistiche di base del morto`, `vite++`, nuova `Run` · *game over definitivo* (nessun fuoco) = `Meta` e `Istanza` azzerati, scheletro dai minimi della prima partita.
La **cronologia** si aggiorna solo se il libro non è bruciato (O3): lo stato "libro intatto" è un flag della `Run`.
**Online (Q):** l'`InstanceStore` è un'interfaccia (`carica(id)`, `applica(delta)`) con **un'unica implementazione su file locale**; la cronologia è un registro in sola aggiunta con id univoci (`autore`, `vita`, `tempo`), quindi fondibile senza conflitti. Un futuro modulo `net/` implementerà la stessa interfaccia su server, **opt-in**, senza telemetria, sanificando i testi altrui.

---

## 3d. La sala come dati: enigmi, nicchie, acqua e raccolta (R1–R6)

Tutto il contenuto della sala sta nella `SceneDef` (`data/gameplay/rooms/sala_segreta.json`): il codice offre i **mattoni**, i dati compongono la stanza.
| Mattone | Descrizione | Esempi nella sala |
|---|---|---|
| `Concealed` | entità/contenitore nascosto finché non viene `reveal` | le 4 nicchie dietro i quadri; il laghetto dietro la porta nord |
| `WaterSource` | acqua `illimitata` o con capacità; riempie contenitori, si beve | il laghetto |
| `Harvestable` | fonte di materia prima: `item`, `porzioni`, ripristino a ogni Run | il muschio sulla parete nord |
| `Station` | postazione di lavorazione con `tipo` e livello di calore | camino acceso = forno/fusione livello 1; alchimia; crogiolo |
| `LootTable` | elenco di oggetti (ID, quantità) generati alla rivelazione | nicchie e porte |
| `PuzzleSlot` | uno degli 8 enigmi (quadro o porta) con condizioni, effetti, loot, `exit` | 4 quadri + 4 porte |
```jsonc
{ "id": "sala_segreta", "exitDoor": "door_west",
  "puzzleSlots": [
    { "id": "p_painting_south", "target": "painting_south", "type": "placeholder_locked",   // enigma da progettare
      "unlock":  { "when": [ /* condizioni ECA */ ], "then": [ {"reveal": "niche_south"} ] },
      "loot":    ["alchemy_kit", "crucible"] },
    { "id": "p_door_north", "target": "door_north", "type": "placeholder_locked",
      "unlock":  { "when": [], "then": [ {"open": "door_north"}, {"reveal": "pond"} ] }, "loot": [] },
    { "id": "p_door_west", "target": "door_west", "exit": true, "type": "placeholder_locked", "unlock": {"when": [], "then": [ {"victory": true} ]} } /* … 8 slot */ ],
  "concealed": [ {"id":"niche_south","behindPainting":"painting_south","contains":"@loot"}, {"id":"pond","behindDoor":"door_north","components":{"WaterSource":{"infinite":true}}} ],
  "harvestables": [ {"id":"moss_north","wall":"north","u":3.0,"item":"moss","portions":5} ] }
```
- **Enigmi come dati:** ogni `PuzzleSlot` ha condizioni (evento–condizione–azione) ed effetti (`reveal`, `open`, `spawn`, `setFlag`, `victory`); un nuovo enigma si scrive senza toccare il codice (A2). Quelli provvisori sono `placeholder_locked` con sblocco di debug (`--unlock <id>`).
- **Uscita:** `exit: true` su una sola porta; l'effetto `victory` termina la partita (K4). Un controllo dati verifica che **esattamente una** porta sia l'uscita.
- **Grafica:** i quadri e le porte hanno stati visivi (`closed`/`open`, `swung`) passati in `RenderScene` (`VisualState`); i mod grafici decidono come mostrarli.
- **Accesso ai quadri (2,95–3,88 m):** non raggiungibili con la salita (1,0 m). Predefinito: l'enigma risolto apre il quadro e fa scendere il loot (decisione 11 in `rules.md`).
- **Cottura e crafting:** il camino acceso è una `Station` di tipo forno; le ricette (impasto, pane) sono `RecipeDef` con `station` e `tempo`.

---

## 4. Moduli, file e livelli

```
main.cpp                             ← argomenti, ciclo; orchestra soltanto
│
ui/ + render/   (backend wireframe_gl: UNICO punto che usa GL per disegnare)
│     backend_wireframe.cpp  models/ (armor, items, furniture, man, dice…)  lighting.cpp  text.cpp  hud.cpp  inventory_ui.cpp  journal_ui.cpp  menu_ui.cpp
│
platform/       (SDL: finestra, input → InputState, audio in futuro)
│
systems/        (NO GL/SDL)   un file per responsabilità, ordine esplicito
│     actions (E/T/Maiusc+E/F, menu) · interaction (bersagli) · physics (gravità, urti, pendolo) ·
│     ballistics (traiettoria condivisa lancio/anteprima) · thermal+reactions (fuoco, acqua, calore, prodotti) ·
│     crafting (ricette, fusione) · status (veleno, scariche, scottature) · needs (sete, fame) · fuel_light (torce, camino, buio) ·
│     combat · movement (salita, accovacciarsi) · lifecycle (morte, rinascita, game over) · ai (SkeletonAI) ·
│     events (bus) → journal · dice_minigame
│
game/           (NO GL/SDL)   dati e stato
│     state.h (GameState = Meta + Run + Istanza) · actor.h · entity.h · defs.h (ObjectDef/MaterialDef/ReactionDef/RecipeDef) ·
│     scene_def (configurazione originale della sala) · instance_store.h (interfaccia) ·
│     inventory · stats · journal · rules/ (soglie lancio, pesi) · save/ (serializzazione, versioni)
│
net/            (FUTURO, opt-in) implementazione remota di InstanceStore; oggi non esiste
│
data/           gameplay/  objects.json materials.json reactions.json recipes.json combat.json needs.json rooms/*.json (SceneDef) puzzles/*.json
│               visual/    models.json lights.json theme.json  + assets/ (mesh, texture, shader)
│
core/ + gfx/    tipi (V, Transform), costanti, primitive wireframe (usate SOLO dal backend wireframe)
mods/<nome>/    sovrascrive/aggiunge file di data/ per ID; caricati in ordine
```
**Livelli:** `main` → `render/ui` → `platform` → `systems` → `game` → `core`. Ogni livello include solo quelli sotto di sé; `systems/` e `game/` non vedono mai `render/ui/platform`.

**Ciclo (ordine esplicito):**
```cpp
InputState in = platform.poll();
actions::apply(S, in);            // E breve/lunga, Maiusc+E, T, F, menu → comandi
physics::step(S, dt);             // gravità, urti, pendoli
thermal::step(S, dt);             // accensioni, combustione, raffreddamento, acqua
crafting::step(S, dt);            // fusioni in corso
needs::step(S, dt);               // sete e fame
fuel_light::step(S, dt);          // consumo di torce e camino; buio → conto alla rovescia
ai::step(S, dt);                  // SkeletonAI → comandi (come il giocatore)
combat::step(S, dt);              // colpi, parate, spinte; movement::step (salita, accovacciarsi)
status::step(S, dt);              // veleno, scariche, scottature
lifecycle::step(S);               // morte → rinascita; game over definitivo; vittoria
events::flush(S);                 // eventi → diario, enigmi (regole ECA)
RenderScene rs = view::build(S);  UiModel ui = view::buildUi(S);
backend->drawScene(rs); backend->drawUi(ui);
```

---

## 5. Le nuove funzioni nell'architettura

| Regola | Dove vive | Note |
|---|---|---|
| **Reazioni** (C1–C3) | `systems/thermal`, `systems/reactions` + `materials.json`/`reactions.json` | Brucia ciò che ha materiale infiammabile; i prodotti (cenere, carbone, macerie) sono entità con massa derivata dal materiale |
| **Calore** (B5) | `Thermal.temperature` | Sostituisce "rovente 10 s": toccare un oggetto sopra soglia scotta (−1 vita) finché si raffredda; le pinze lo permettono |
| **Lancio con T** (D2, D3, E2) | `systems/ballistics` + `rules/throw.json` | Un'unica funzione integra la traiettoria: usata per l'**anteprima** (arco di punti in `RenderScene.aimPreview`) e per il lancio reale → nessuna divergenza |
| **Soglie dei dadi** (E2) | `game/rules` | Tabella dati: soglia → bonus energia / mira assistita |
| **Macerie e ceneri** (B4) | entità con materiale `ash`/`rubble` | Raccoglibili, spostabili; ingredienti di ricette |
| **Fusione metalli** (G2) | `systems/crafting` + `recipes.json` | Stazione (crogiolo) + fonte di calore a livello sufficiente (camino=1, forgia con mantice=2) → lingotti → ricette di forgia |
| **Scheletro** (F1) | `Entity` con `Stats` nascoste e `Anim=false` | Statistiche = minimo dei tre lanci, salvate, mai mostrate; nessun sistema le legge finché `Anim` non diventa vero |
| **Sedia/scheletro mobili** (F2, F3) | normali entità con massa | Le braccia dello scheletro restano doppio pendolo (fisica attuale) |
| **Libro/diario** (H1, H2, E3) | `game/journal` + `ui/journal_ui` + bus eventi | Voci automatiche dagli eventi; testo via `InputState.text` (UTF-8) |
| **Menu e salvataggio** (I1, I2) | `game/save` + `ui/menu_ui` | `GameState` serializzabile (JSON versionato), migrazioni tra versioni, cartella XDG locale |
| **Enigmi come dati** | `systems/events` + `puzzles/*.json` | Regole evento–condizione–azione (es. "se la chiave è usata sulla porta X → apri X, annota nel diario") |
| **Ciclo roguelike** (K1–K5) | `systems/lifecycle` + `game/scene_def` + `Meta` | La morte ricrea la `Run` dalla `SceneDef`; lo scheletro prende le statistiche di base del morto; niente `execv` |
| **Sopravvivenza** (L1, L2) | `systems/needs` + `needs.json` | Sete e fame come componente di `Actor`; cibo e acqua sono oggetti normali con un effetto "consumabile" |
| **Torce e camino che si consumano** (M1, M2) | `Thermal.fuel` + `systems/fuel_light` | Il combustibile è una proprietà del materiale (kg × secondi/kg); la luce emessa scende con il combustibile |
| **Buio e game over** (M4, K5) | `systems/fuel_light` + `systems/lifecycle` | Nessuna fonte di luce → `Countdown` (clessidra) → `GameOverDefinitivo` |
| **Oggetti roventi** (M3) | `Thermal.temperature` | Non inceneribili nel fuoco si scaldano e si raffreddano; sotto soglia si possono toccare |
| **Scrigno-inventario** (N1) | `Container` con `slots=6`, `maxSizeClass=3`, `volumeBudget=12` | Stesso `Container` dell'inventario del giocatore: stessa UI, stessi comandi |
| **Scheletro attore** (O1, O2, O6) | `Actor` + `SkeletonAI` | Risvegliato dall'evento "libro bruciato"; ha `canCraft=false`, `canCast=false` |
| **Combattimento e movimento** (O4, O5) | `systems/combat`, `systems/movement`, `combat.json` | Comandi `Slash/Thrust/Parry/Push`; confronto statistiche in background |
| **Clessidra globale** (P1) | `UiModel.timers` | Ogni `Countdown` ha una clessidra; il backend la disegna (sostituibile) |
| **Libro dopo il rogo** (O3) | `Run.bookIntact` | Se falso, `events::journal` non scrive più |
| **Istanze condivise** (Q1, Q2) | `InstanceStore` (+ futuro `net/`) | Oggi un file locale; stessa interfaccia per il server |

---

## 6. Moddabilità: chi cambia cosa

| Vuoi cambiare… | Tocchi | Non devi toccare |
|---|---|---|
| Peso, materiale, infiammabilità di un oggetto | `data/gameplay/objects.json`, `materials.json` | codice, grafica |
| Una ricetta o una soglia di lancio | `recipes.json`, `rules/throw.json` | codice, grafica |
| Un enigma | `puzzles/*.json`, `rooms/*.json` | grafica |
| L'aspetto di un oggetto (mesh/texture) | `data/visual/models.json` + `assets/` | logica |
| Illuminazione, shader, tema dell'HUD | backend o `lights.json`/`theme.json` | logica |
| Un'intera resa grafica nuova | un nuovo `IRenderBackend` | `game/`, `systems/` |
| Una regola nuova (nuovo agente, nuovo sistema) | `systems/` + `game/` | grafica (solo, se serve, un nuovo `Effect` in `RenderScene`) |

I **mod** sono cartelle `mods/<nome>/` che sovrascrivono o aggiungono file per ID. Solo **dati** (niente codice eseguibile) per sicurezza; lo scripting Lua è un'estensione futura opzionale.

---

## 7. Prestazioni
- Contatori di frame (vertici, chiamate, ms) sempre disponibili.
- **Display list** (o VBO) della geometria statica **dentro il backend wireframe** (dettaglio d'implementazione: non tocca la logica). Obiettivo: da ~1.900 `glBegin`/frame a poche centinaia.
- `view::build` riusa i vettori (niente allocazioni per frame); etichette costruite solo per il bersaglio.
- Fisica con passo fisso e soglia di riposo (gli oggetti fermi non vengono simulati).
- Pipeline fissa mantenuta (funziona su Arch/X11/NVIDIA); il backend `mesh_gl` con shader è un'estensione.

## 8. Build e test
- **CMake**, una libreria per modulo, eseguibile unico; Makefile di riserva. Ricompilazione incrementale ~1 s.
- **Test headless** (`tests/`, senza SDL/GL): pesi e capacità, equipaggiamento, soglie di lancio, traiettoria (anteprima = lancio), reazioni materiale × agente, ricette, serializzazione (salva→carica→stesso stato), diario.
- **Regressione visiva**: screenshot `--shot` deterministici (tempo fisso, seme fisso) confrontati pixel per pixel a ogni passo.

---

## 9. Come cambia il lavoro quotidiano
| Sviluppo | Oggi | Dopo |
|---|---|---|
| Nuovo oggetto | ~12 punti in un file da 2142 righe | una voce in `objects.json` (+ eventuale modello in `visual/`) |
| Nuovo materiale o reazione | codice sparso | una voce in `materials.json` / `reactions.json` |
| Nuova ricetta o enigma | codice | un file dati |
| Nuova resa grafica | impossibile senza toccare la logica | nuovo backend/asset |
| Nuovo menu | `main()` | un `MenuDef` + `UiModel` |

In una nuova chat: allegare `changelog.md`, `rules.md`, `architecture.md` e **solo i file interessati**.

---

## 10. Roadmap v3 — Parte I: architettura (difficoltà crescente)

Regola d'oro: ogni passo termina con un programma che **compila e produce screenshot identici** a quelli di riferimento (salvo il passo che introduce una funzione). Un passo = una o due chat. Nessun passo mescola refactoring e nuove funzioni.

| # | Passo | Diff. | Rischio | Regole | Fatto quando |
|---|---|---|---|---|---|
| M0 ✅ | **Rete di sicurezza** *(completato)*: `--deterministic` (passo 1/60 s, seme fisso; implicito con `--shot`), `tools/` con 17 scenari e confronto pixel per pixel; copia `archive/sala_segreta_v0.7.cpp` | ★ | molto basso | J2 | **verificato:** due esecuzioni → 17/17 screenshot identici |
| M1 ✅ | **Primitive** *(completato)*: `core/types.h`, `core/config.h`, `gfx/primitives.*` | ★★ | basso | — | **verificato:** 17/17 screenshot identici a M0; nessun avviso |
| M2 ✅ | **Build multi-file** *(completato)*: `Makefile` + `CMakeLists.txt`; stanza, arredi, modelli, luci in `src/render/*.cpp`; stato condiviso via `state.h` con `extern` (temporaneo); fisica dello scheletro in `skeleton_physics.*` | ★★ | basso | — | **verificato:** build pulita senza avvisi (make e CMake), 17/17 screenshot identici; incrementale 0,6 s per un file di render |
| M3 ✅ | **Testo e UI** *(completato)*: `ui/text`, `draw2d`, `figure`, `inventory_ui`, `hourglass` (`Countdown`, regola P1); `items.h/.cpp` condiviso; `render/viewmodel` | ★★★ | basso–medio | P1 | **verificato:** 17/17 screenshot identici, 0 avvisi; monolite 969 righe |
| M4 *(a, b, c ✅)* | **Modello a oggetti** (tabelle C++ per ora): `ObjectDef`, `MaterialDef`, `Entity` con componenti; `Item`/`Loose` unificati; arredi (tavolo, sedia, scheletro, rastrelliera) e contenuti diventano entità; **il giocatore diventa un `Actor` (lo scheletro un secondo)**; via i 13 `switch` | ★★★★ | medio–alto | B1, B6, O1 | un solo concetto di oggetto; screenshot e pesi invariati. **Sotto-passi:** **M4a ✅** tabelle `ObjectDef`/`MaterialDef` (`game/defs`), test dei pesi · **M4b ✅** entità unica (`Item`+`Loose` → `Entity`) · **M4c** arredi e contenuti come entità, in due tranche: **c ✅** scrigno e secchio con contenuto, arredi e appesi (`g_fixtures`) · **M4d** `Actor` (giocatore, scheletro) · **M4e** bersagli senza `HOT[]`/`kind` |
| M5 | **`GameState` = Meta + Run + Istanza**: le 35 globali diventano campi; `SceneDef` (configurazione originale della sala) come struttura; la morte ricrea la Run, niente `execv` | ★★★★ | medio–alto | I2, I3, K1 (base) | nessuna `g_*` rimasta; morte → Run ricreata identica |
| M6 | **Sistemi al posto del `main()`**: input/azioni, fisica, termica, status, minigioco con ordine esplicito; `main` < 150 righe | ★★★★ | alto | A1 (metà) | comportamento invariato |
| M7 | **Confine logica/grafica**: `RenderScene`, `UiModel` (con `timers`), `InputState`, `ActorPose`, `IRenderBackend`; il wireframe diventa il primo backend; `game/` e `systems/` senza GL/SDL | ★★★★★ | alto | **A1, A3, A4** | `grep -r "GL/\|SDL" game systems` = vuoto; immagini identiche |
| M8 | **Interagibili e menu dichiarativi** (`Interactable`, `MenuDef`) al posto di `kind` 0–7 e `menu` 1–4 | ★★★★ | alto | — | stesse interazioni di prima |
| M9 | **Ottimizzazione nel backend**: display list/VBO per la geometria statica, contatori di frame | ★★★ | medio | — | `glBegin`/frame ridotti, aspetto identico |
| M10 | **Serializzazione dello stato** (JSON versionato, solo infrastruttura) | ★★★ | medio | I2 | salva→carica→stato identico (test) |
| M11 | **Dati su file**: oggetti, materiali, reazioni, ricette, **`SceneDef` della sala** caricati da `data/`; sistema di mod per ID | ★★★★ | medio–alto | **A2**, C4, G1 | modificare un JSON cambia il gioco senza ricompilare |
| M12 | **Test headless** dei sistemi e delle regole | ★★★ | basso | J1 | i test girano senza SDL/GL |

*Differenze rispetto alla v1:* il vecchio passo 4 (tabella oggetti) diventa **M4 (modello a oggetti)**, più ampio; **M7 (confine logica/grafica)** è nuovo e centrale; l'ottimizzazione passa **dopo** il confine (è un dettaglio del backend); **M10–M11** sono nuovi.

## 11. Perché questo ordine
- M0–M3 sono **meccanici** (spostare codice) e danno subito compilazione rapida.
- **M4–M7 vanno fatti prima delle nuove funzioni**: reazioni, lancio, crafting e diario scritti nel vecchio monolite andrebbero riscritti due volte.
- M7 è il passo più difficile e quello che rende vere le regole A1–A4: da lì, grafica e logica evolvono separatamente.
- M10–M12 abilitano salvataggio, mod e test.

## 12. Roadmap v3 — Parte II: funzioni (dopo M7; M10–M12 dove indicato)

| # | Funzione | Diff. | Dipende da | Regole |
|---|---|---|---|---|
| F1 | **Tutti gli oggetti con fisica**: tavolo, sedia, scheletro, scrigno, secchio spostabili/lanciabili entro peso e Forza | ★★★★ | M4–M7 | B1–B3, F2 |
| F2 | **Reazioni**: incenerimento (tavolo, scrigno, secchio, sedia…), prodotti ceneri/macerie, **acqua** (spegne, bagna), calore come proprietà | ★★★★ | F1, M11 consigliato | C1–C3, B5, F3 |
| F3 | **Lancio con T**: anteprima della traiettoria, formula energia/massa/Forza, soglie dei dadi, dispersione con Destrezza | ★★★ | F1, M7 | D2, D3, E2 |
| F4 | **Macerie e ceneri** raccoglibili; **crafting** di base | ★★★ | F2, M11 | B4, G1 |
| F5 | **Fusione dei metalli**: crogiolo, stampo, pinze, mantice; livelli di calore | ★★★★ | F2, F4 | G2 |
| F6 | **Scheletro**: statistiche proprie nascoste (alla prima partita i minimi dei lanci; poi ereditate, F11) e stato inerte/animato | ★★★ | M4, M10 | F1, K2 |
| F7 | **Libro-diario**: nome, note (inserimento testo), cronologia delle scoperte (bus eventi) | ★★★ | M7, M10 | E3, H1, H2 |
| F8 | **Menu Esc e salvataggio/caricamento** (slot, autosalvataggio) | ★★★★ | M10 | I1, I2 |
| F9 | **Prova di moddabilità grafica**: backend alternativo di esempio (mesh/texture/shader) e un mod dimostrativo | ★★★★★ | M7, M11 | A3, A4 |
| F10 | **Framework degli enigmi come dati** (ECA): 8 `PuzzleSlot` (4 quadri + 4 porte), effetti `reveal/open/spawn/setFlag/victory`, porte apribili, un'unica uscita, sblocco di debug | ★★★★ | M8, M11 | A2, K4, R6 |
| F11 | **Ciclo roguelike**: morte → rinascita con nuovi dadi, sala ripristinata, scheletro che eredita, libro persistente, game over definitivo | ★★★★ | M5, M11, F6, F7 | K1–K5, O3 |
| F12 | **Sopravvivenza**: sete e fame, cibo e acqua come oggetti, fonte d'acqua, tasto uso | ★★★ | M4, F2 | L1, L2 |
| F13 | **Combustibile e luce**: torce e camino che si consumano, legna, buio → conto alla rovescia → game over definitivo | ★★★ | F2, M11 | M1, M2, M4, K5 |
| F14 | **Oggetti incombustibili nel fuoco**: diventano roventi e si raffreddano; clessidra | ★★ | F2 | M3 |
| F15 | **Scrigno come secondo inventario**: 6 slot, classi di dimensione, pannello deposito/prelievo | ★★★ | M3, M4 | N1 |
| F16 | **Scheletro attore**: rig procedurale, risveglio, IA, stessi comandi del giocatore (armi, armature, trasporto, lancio) | ★★★★★ | M6, M7, F1, F3 | O1, O2, O6 |
| F17 | **Combattimento in tempo reale**: fendente, affondo, parata, spinta, scudo; confronto statistiche e danni | ★★★★★ | F16 | O4, O5 |
| F18 | **Movimento esteso**: Maiusc+W salita (tavoli, sedie, scale), Ctrl accovacciarsi, collisioni 3D | ★★★★ | M4, F1 | O4 |
| F19 | **Istanza e online (solo predisposizione)**: `InstanceStore` su file, registro d'istanza con autori, versione regole, sanificazione | ★★★ | M5, M10, F7 | Q1, Q2 |
| F20 | **Abilità**: crafting e incantesimi come conoscenza del giocatore (non ereditata) — sistema degli incantesimi da progettare | ★★★★★ | F4, F7 | K6, O6 |
| F21 | **Contenuti della sala**: 4 nicchie dietro i quadri, laghetto dietro la porta nord (acqua illimitata), muschio raccoglibile, attrezzatura alchemica e crogiolo, pane e farina, loot delle porte; 8 `PuzzleSlot` provvisori sbloccabili in debug | ★★★ | F10, F2, F4, F5, F12 | R1–R6 |

**Ordine consigliato (v3):** M0 → M7 (+M8, M9) → M10, M11 (almeno `SceneDef`) → **F1 → F3 → F7 → F11** (primo ciclo di gioco completo: muori, rinasci, il libro ricorda) → F2 → F13 → F14 → F12 → F15 → F8 → F19 → F18 → F4 → F5 → **F16 → F17** → F10 → **F21** → F20 → F9.
**Prototipo di base completo (versione 1.0 giocabile):** M0–M11, F1–F3, F7, F8, F11–F15, F10, F21. Attori attivi e combattimento (F16–F18) sono la seconda grande tappa.

---

## 13. Rischi e mitigazioni
| Rischio | Mitigazione |
|---|---|
| Portata molto ampia (fisica, reazioni, crafting, diario, salvataggio, mod, sopravvivenza, combattimento) | Roadmap a passi misurabili; prototipo giocabile = M0–M11 + F1–F3, F7, F8, F11–F15, F10; combattimento e attori in una seconda tappa |
| **F16–F17 (scheletro attivo e combattimento)** sono i pezzi più costosi: rig articolato procedurale, IA, animazioni e tempi di combattimento da tarare | Dipendono da M4–M7 (Actor, comandi, `ActorPose`); si progettano `Command` e `ActorPose` già in M4/M7 anche se usati dal solo giocatore |
| Bilanciamento di bisogni, combustibile e buio (partite troppo facili o impossibili) | Tutti i valori in `needs.json`/`materials.json`; modalità di prova con tempo accelerato (`--timescale`) |
| Il giocatore "non singleton" tocca molte righe | Si fa in M4–M5 dentro la rete di screenshot, prima di aggiungere altre funzioni |
| Online: rischio di sovra-ingegnerizzare | Solo interfaccia + registro d'istanza locale (F19); nessun codice di rete |
| M7 (confine logica/grafica) è invasivo | Si fa dopo M4–M6, con la rete di screenshot, e con un backend che replica esattamente quello attuale |
| Divergenza tra anteprima e lancio | Una sola funzione di integrazione (`ballistics`), testata |
| Salvataggi rotti dai cambi di formato | Campo `version` e migrazioni, test salva→carica |
| Mod che rompono il gioco | Validazione dei dati al caricamento con messaggi d'errore chiari; solo dati |
| Differenze visive per il tempo (fiamme) | M0: tempo e seme deterministici in `--shot` |

## 14. Protocollo di lavoro
1. Prima di ogni passo: build e screenshot di riferimento; dopo: confronto.
2. Un passo per volta; mai refactoring e funzione nuova insieme.
3. Ogni sviluppo cita gli ID di `rules.md` nel `changelog.md`; a fine passo si aggiornano changelog, mappa dei file e la tabella dello stato in `rules.md`.
4. Le **decisioni aperte** (in fondo a `rules.md`) hanno valori predefiniti: si cambiano modificando quel file prima del passo che le riguarda.
