// Oggetti dell'inventario: dati e funzioni sui pesi e sulle statistiche (vedi items.h).
#include "items.h"
#include "game/defs.h"
#include "core/config.h"
#include <algorithm>
#include <cstdio>

Entity g_inv[9], g_hand[2];
bool g_equip[10] = {};
int g_stats[5] = {0, 0, 0, 0, 0};
int g_hp = 1;

const int SLOT_TYPE[10] = {D_HELM, D_CUIRASS, D_PAULDRON, D_PAULDRON, D_VAMBRACE, D_VAMBRACE, D_GREAVE, D_GREAVE, D_BOOT, D_BOOT};
const char* const SLOT_NAME[10] = {"Elmo", "Corazza", "Spallina sinistra", "Spallina destra", "Bracciale sinistro", "Bracciale destro",
                                    "Gambale sinistro", "Gambale destro", "Stivale sinistro", "Stivale destro"};
const char* const STAT_NAMES[5] = {"Forza", "Destrezza", "Magia", "Carisma", "Vita"};
const int STAT_MOD[5] = {2, -1, -1, 0, 2};              // per ogni pezzo d'armatura indossato: Forza e Vita +2, Destrezza e Magia -1
std::string fmtKg(float k) { char b[32]; snprintf(b, sizeof b, "%.1f", k); std::string r = b; for (auto& c : r) if (c == '.') c = ','; return r; }

std::string itemName(const Entity& it) {
    const ObjectDef& d = defOf(it.def);
    if (!(d.flags & F_INVENTORY)) return "";
    if (it.def == D_BUCKET && holds(it, D_KEY)) return "Secchio di cenere (da frugare)";     // finché la chiave è dentro
    return (it.lit && d.nameLit) ? d.nameLit : d.name;
}

float itemKg(int t) { return defOf(t).kg; }

const char* defArticle(int def) { return defOf(def).id == D_NONE ? "il pezzo" : defOf(def).article; }

float invKg() { float w = 0; for (auto& s : g_inv) w += itemKg(s.def); return w; }

float handKg() { return itemKg(g_hand[0].def) + itemKg(g_hand[1].def); }

float wornKg() { float w = 0; for (int i = 0; i < 10; i++) if (g_equip[i]) w += itemKg(SLOT_TYPE[i]); return w; }

float totalKg() { return invKg() + handKg() + wornKg() + (g_carrying ? itemKg(g_carry.def) : 0.f); }

Entity slotOf(const Entity& o) {
    if (!defHas(o.def, F_INVENTORY)) return Entity{};     // scrigno, ceneri, sconosciuti: non inventariabili
    Entity e; e.def = o.def; e.lit = defHas(o.def, F_LIGHTABLE) ? o.lit : false;
    e.contents = o.contents;
    return e;
}

int armorCount() { int n = 0; for (bool b : g_equip) n += b ? 1 : 0; return n; }

int effStat(int i) { int n = ARMOR_BONUS_PER_PIECE ? armorCount() : (armorCount() > 0 ? 1 : 0); return std::max(0, g_stats[i] + STAT_MOD[i] * n); }

int hpMax() { return std::max(1, effStat(4)); }                      // la Vita (con i modificatori) è il massimo dei punti vita
