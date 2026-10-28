// Oggetti dell'inventario e regole sui pesi/statistiche (interim del passo M3; diventerà `game/inventory` e `game/stats`, M4–M5).
// Qui sta tutto ciò che sia la logica sia l'interfaccia devono conoscere dell'inventario.
#pragma once
#include <string>
#include "state.h"
#include "game/defs.h"
#include "game/entity.h"


extern Entity g_inv[9], g_hand[2];                          // inventario; g_hand[0] = mano sinistra, [1] = destra
extern bool g_equip[10];                                  // indossati: elmo, corazza, spallina s/d, bracciale s/d, gambale s/d, stivale s/d
extern int g_stats[5];                                    // Forza, Destrezza, Magia, Carisma, Vita (valori di base, dai dadi)
extern int g_hp;                                          // punti vita correnti

extern const int SLOT_TYPE[10];                           // tipo di oggetto accettato da ogni slot di equipaggiamento
extern const char* const SLOT_NAME[10];
extern const char* const STAT_NAMES[5];
extern const int STAT_MOD[5];                             // modificatori dell'armatura: Forza e Vita +2, Destrezza e Magia -1
// Bonus/malus: true = per ogni pezzo indossato; false = una sola volta se si indossa almeno un pezzo
inline constexpr bool ARMOR_BONUS_PER_PIECE = false;

inline bool isArmor(int t) { return defHas(t, F_WEARABLE); }
std::string fmtKg(float k);                               // "7,3"
std::string itemName(const Entity& it);
float itemKg(int type);                                   // peso di un tipo di oggetto (kg), dalla tabella delle definizioni
Entity slotOf(const Entity& o);                           // copia "pulita" per una mano o uno slot (def, lit e contenuto); vuota se non è inventariabile
const char* defArticle(int def);                          // nome con articolo ("la corazza", "una spallina")
float invKg(); float handKg(); float wornKg(); float totalKg();     // pesi: inventario, mani, indossato, totale (con l'oggetto trasportato)
int armorCount();                                         // pezzi d'armatura indossati
int effStat(int i);                                       // caratteristica con i modificatori dell'armatura
int hpMax();                                              // punti vita massimi = Vita effettiva (almeno 1)
