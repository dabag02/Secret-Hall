// Tabelle delle definizioni di oggetti e materiali (vedi defs.h). I pesi e i nomi sono quelli già in uso nel gioco (v0.7).
#include "game/defs.h"
#include "core/config.h"     // SKELETON_KG

static const MaterialDef MATERIALS[M_COUNT] = {
    {M_NONE,   "none",   "(nessuno)", 0.f,   false, 0},
    {M_WOOD,   "wood",   "legno",     0.6f,  true,  0},
    {M_PAPER,  "paper",  "carta",     0.8f,  true,  0},
    {M_STEEL,  "steel",  "acciaio",   7.85f, false, 2},
    {M_BRONZE, "bronze", "bronzo",    8.8f,  false, 1},
    {M_GOLD,   "gold",   "oro",       19.3f, false, 1},
    {M_GEM,    "gem",    "gemma",     3.5f,  false, 0},      // densità provvisoria (non in rules.md)
    {M_ASH,    "ash",    "cenere",    0.5f,  false, 0},      // densità provvisoria (non in rules.md)
    {M_BONE,   "bone",   "osso",      1.9f,  false, 0},
};

// Materiale dei pezzi d'armatura: provvisorio (acciaio), da confermare con il crafting/fusione (F5).
static const unsigned ARM = F_CARRIABLE | F_INVENTORY | F_WEARABLE;
static const ObjectDef DEFS[D_COUNT] = {
    {D_NONE,     "none",     "",                         nullptr,         "l'oggetto",       0.f,   M_NONE,   0,                                          ""},
    {D_KEY,      "key",      "Chiave di bronzo",         nullptr,         "la chiave",       .15f,  M_BRONZE, F_CARRIABLE | F_INVENTORY,  "key"},
    {D_TORCH,    "torch",    "Torcia spenta",            "Torcia accesa", "la torcia",       1.2f,  M_WOOD,   F_CARRIABLE | F_INVENTORY | F_LIGHTABLE,  "torch"},
    {D_SWORD,    "sword",    "Spada lunga",              nullptr,         "la spada",        1.6f,  M_STEEL,  F_CARRIABLE | F_INVENTORY,  "sword"},
    {D_SHIELD,   "shield",   "Scudo con croce dorata",   nullptr,         "lo scudo",        3.0f,  M_WOOD,   F_CARRIABLE | F_INVENTORY,  "shield"},
    {D_BUCKET,   "bucket",   "Secchio di legno",         nullptr,         "il secchio",      3.0f,  M_WOOD,   F_CARRIABLE | F_INVENTORY | F_CONTAINER,  "bucket"},
    {D_GEM,      "gem",      "Gemma",                    nullptr,         "la gemma",        .1f,   M_GEM,    F_CARRIABLE | F_INVENTORY,  "gem"},
    {D_COINS,    "coins",    "Monete d'oro",             nullptr,         "le monete d'oro", .9f,   M_GOLD,   F_CARRIABLE | F_INVENTORY,  "coins"},
    {D_BOOK,     "book",     "Libro",                    nullptr,         "il libro",        1.2f,  M_PAPER,  F_CARRIABLE | F_INVENTORY,  "book"},
    {},                                                                                                                  // 9: libero
    {D_HELM,     "helm",     "Elmo",                     nullptr,         "l'elmo",          1.8f,  M_STEEL,  ARM,  "armor_helm"},
    {D_CUIRASS,  "cuirass",  "Corazza",                  nullptr,         "la corazza",      5.0f,  M_STEEL,  ARM,  "armor_cuirass"},
    {D_PAULDRON, "pauldron", "Spallina",                 nullptr,         "una spallina",    1.0f,  M_STEEL,  ARM,  "armor_pauldron"},
    {D_GREAVE,   "greave",   "Gambale",                  nullptr,         "un gambale",      1.5f,  M_STEEL,  ARM,  "armor_greave"},
    {D_VAMBRACE, "vambrace", "Bracciale",                nullptr,         "un bracciale",    .8f,   M_STEEL,  ARM,  "armor_vambrace"},
    {D_BOOT,     "boot",     "Stivale",                  nullptr,         "uno stivale",     1.2f,  M_STEEL,  ARM,  "armor_boot"},
    {},{},{},{},                                                                                                          // 16..19: liberi (l'indice nella tabella è l'id: lo verifica test_defs)
    {D_CHEST,    "chest",    "Scrigno",                  nullptr,         "lo scrigno",      12.f,  M_WOOD,   F_CARRIABLE | F_CONTAINER,  "chest"},      // non inventariabile
    {D_ASH,      "ash",      "Ceneri tossiche",          nullptr,         "le ceneri tossiche", 0.f, M_ASH,   0,  "ash"},
    {},{},{},{},{},{},{},{},                                                                                                // 22..29: liberi
    // Arredi e supporti. Le masse di tavolo, sedia, rastrelliera e ganci NON sono ancora decise (0 = ignota): le fisseranno gli oggetti spostabili (F1).
    {D_TABLE,    "table",    "Tavolo",                   nullptr,         "il tavolo",       0.f,   M_WOOD,   F_FIXED,                                    "table"},
    {D_CHAIR,    "chair",    "Sedia",                    nullptr,         "la sedia",        0.f,   M_WOOD,   F_FIXED,                                    "chair"},
    {D_SKELETON, "skeleton", "Scheletro",                nullptr,         "lo scheletro",    SKELETON_KG, M_BONE, F_FIXED,                                "skeleton"},
    {D_RACK,     "rack",     "Rastrelliera",             nullptr,         "la rastrelliera", 0.f,   M_WOOD,   F_FIXED | F_CONTAINER,                      "rack"},
    {D_SWORD_HOOKS,  "sword_hooks",  "Ganci della spada",  nullptr,       "i ganci della spada", 0.f, M_STEEL, F_FIXED | F_CONTAINER,                   "sword_hooks"},
    {D_SHIELD_HOOKS, "shield_hooks", "Ganci dello scudo",  nullptr,       "i ganci dello scudo", 0.f, M_STEEL, F_FIXED | F_CONTAINER,                   "shield_hooks"},
};

const ObjectDef& defOf(int id) { return (id > 0 && id < D_COUNT && DEFS[id].id == id) ? DEFS[id] : DEFS[0]; }
const MaterialDef& materialOf(MaterialId id) { return MATERIALS[(id >= 0 && id < M_COUNT) ? id : 0]; }
