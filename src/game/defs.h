// Definizioni statiche di oggetti e materiali (passo M4a). Livello `game/`: nessuna dipendenza da OpenGL o SDL.
// Qui sta tutto ciò che *non cambia durante il gioco*: nome, articolo, peso, materiale, flag, ID visivo.
// Oggi sono tabelle C++; al passo M11 diventeranno `objects.json` / `materials.json` con la stessa struttura (A2).
// Regola A3: il gameplay conosce solo `visualId` (una stringa), mai mesh, colori o funzioni di disegno.
#pragma once

// Identificatori delle definizioni (sono anche i valori di `Entity::def`). I valori 1..15 sono quelli storici dei tipi d'inventario; 20 e 21 sono oggetti non inventariabili (scrigno e ceneri).
enum DefId : int {
    D_NONE = 0,
    D_KEY = 1, D_TORCH, D_SWORD, D_SHIELD, D_BUCKET, D_GEM, D_COINS, D_BOOK,
    D_HELM = 10, D_CUIRASS, D_PAULDRON, D_GREAVE, D_VAMBRACE, D_BOOT,
    D_CHEST = 20, D_ASH,
    // arredi e supporti (sempre nella sala, non si raccolgono: F_FIXED)
    D_TABLE = 30, D_CHAIR, D_SKELETON, D_RACK, D_SWORD_HOOKS, D_SHIELD_HOOKS,
    D_COUNT
};

enum MaterialId : int { M_NONE = 0, M_WOOD, M_PAPER, M_STEEL, M_BRONZE, M_GOLD, M_GEM, M_ASH, M_BONE, M_COUNT };

struct MaterialDef {
    MaterialId id;
    const char* key;
    const char* name;
    float density;          // g/cm³ (da rules.md; quelli marcati "provvisorio" in defs.cpp non sono ancora stati decisi)
    bool flammable;         // brucia (B/C1)
    int meltLevel;          // livello di calore per fonderlo (G2): 0 = non fusibile; 1 = camino acceso; 2 = forgia con mantice
};

enum : unsigned {
    F_CARRIABLE = 1u << 0,  // si può sollevare e trasportare (E tenuto)
    F_INVENTORY = 1u << 1,  // entra nell'inventario o nelle mani
    F_WEARABLE  = 1u << 2,  // si indossa (slot = la propria definizione, vedi SLOT_TYPE in items.cpp)
    F_CONTAINER = 1u << 3,  // può contenere altri oggetti (B6, N1)
    F_LIGHTABLE = 1u << 4,  // ha uno stato acceso/spento (`lit`)
    F_FIXED     = 1u << 5,  // arredo o supporto: sempre in sala, non si raccoglie né si sposta (per ora; F1 li renderà spostabili)
};

struct ObjectDef {
    DefId id;
    const char* key;        // identificativo stabile per i dati e i salvataggi ("torch")
    const char* name;       // nome mostrato nell'inventario ("Torcia spenta")
    const char* nameLit;    // nome quando è acceso; nullptr = uguale a `name`
    const char* article;    // forma con articolo, per le frasi ("la torcia", "una spallina")
    float kg;               // massa (kg)
    MaterialId material;
    unsigned flags;
    const char* visualId;   // collegamento al backend grafico (A3)
};

const ObjectDef& defOf(int id);                  // D_NONE per id sconosciuti (mai nullptr)
const MaterialDef& materialOf(MaterialId id);
inline bool defHas(int id, unsigned flag) { return (defOf(id).flags & flag) != 0; }
