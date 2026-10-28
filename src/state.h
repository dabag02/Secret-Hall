// Stato condiviso tra logica e disegno (interim del passo M2).
// Verrà sostituito da `GameState` (Meta + Run + Istanza) al passo M5; fino ad allora le variabili usate dai file di render/
// sono definite in state.cpp e dichiarate qui con `extern`. Il resto dello stato resta nel file principale.
#pragma once
#include <vector>
#include "core/types.h"
#include "game/entity.h"

// --- stanza: pareti (riferimento locale: x lungo il muro, y alto, z verso l'interno) ---
struct Hole { float u0, u1, v1; };
struct Wall { float ox, oz, ux, uz, nx, nz; std::vector<Hole> holes; };
extern Wall W[4];                     // 0 Nord, 1 Sud, 2 Est, 3 Ovest
extern const float DOOR_U[4];         // posizione delle porte lungo ogni parete
extern const float TORCH_U[4];        // posizione delle torce a muro lungo ogni parete

// --- torce a muro ---
struct WallTorch { bool present, lit; };
extern WallTorch g_wt[4];

// --- oggetti nel mondo (secchio, scrigno, elmo e ciò che viene lasciato cadere): stessa struct dell'inventario (M4b) ---
extern std::vector<Entity> g_loose;
extern bool g_carrying;
extern Entity g_carry;

// --- stato di arredi e ambiente ---
extern bool g_fireLit;
extern int g_zapDoor;
extern float g_zapT, g_time;                  // g_time = tempo di gioco (s), usato dalle animazioni

// --- arredi e supporti (M4c): entità fisse della sala. Lo stato che prima stava in globali sparsi (tavolo e rastrelliera in fiamme,
//     pezzi appesi, spada e scudo ai ganci, braccia dello scheletro) vive ora nell'entità. L'elenco è stato di gioco (parte della Run, M5). ---
extern std::vector<Entity> g_fixtures;
extern const int RACK_TYPE[9];                           // che cosa va in ciascun posto della rastrelliera (corazza, spalline s/d, gambali s/d, bracciali s/d, stivali s/d)
std::vector<Entity> makeFixtures();                      // gli arredi nella configurazione originale (nuova Run)
Entity& fixture(int def);                                     // l'arredo di quel tipo (deve esistere)
inline int tableState() { return fixture(D_TABLE).fireState; }
inline int rackState() { return fixture(D_RACK).fireState; }
int hungIndex(const Entity& support, int slot);          // posizione nel contenuto dell'oggetto appeso al posto `slot`, -1 se vuoto
inline bool hungAt(const Entity& support, int slot) { return hungIndex(support, slot) >= 0; }
inline bool rackHas(int slot) { return hungAt(fixture(D_RACK), slot); }
inline bool swordOnWall() { return !fixture(D_SWORD_HOOKS).contents.empty(); }
inline bool shieldOnWall() { return !fixture(D_SHIELD_HOOKS).contents.empty(); }
void hang(Entity& support, Entity item, int slot);       // appende `item` al posto `slot` (l'oggetto perde posizione e calore)
Entity unhang(Entity& support, int index);               // stacca e restituisce l'oggetto in posizione `index` del contenuto
int spillSupport(Entity& support, std::vector<Entity>& world, const V* slotPos, float hot);   // crollo: tutto ciò che è appeso cade nel mondo (posizione = slotPos[slot]); restituisce quanti
