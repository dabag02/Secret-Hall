// L'oggetto: un solo concetto per tutto ciò che si può avere in mano, nell'inventario o a terra (passo M4b).
// Livello `game/`: nessuna dipendenza da OpenGL o SDL.
// `def` dice *che cosa* è (tabella in game/defs); il resto è stato *variabile*. In una mano o in uno slot dell'inventario contano
// solo `def` e `lit`: posizione, calore e combustione valgono solo per un oggetto nel mondo. Con M4c l'entità avrà componenti
// (Physical, Thermal, Container…) al posto di questi campi.
#pragma once
#include <vector>
#include "core/types.h"
#include "game/defs.h"

struct Entity {
    int def = D_NONE;        // un DefId; D_NONE = vuoto (slot libero)
    bool lit = false;        // acceso (solo oggetti F_LIGHTABLE: le torce)
    V pos{0, 0, 0};          // posizione nel mondo (m)
    float yaw = 0, vy = 0;   // orientamento (rad) e velocità verticale (m/s)
    float hot = 0, burn = 0; // s di calore residuo (M3); s di combustione residua (libro)
    // Contenitore (B6, N1; secchio e scrigno): gli oggetti dentro sono entità vere e viaggiano con il contenitore
    // (in mano, nell'inventario, trasportato, lasciato cadere). Diventerà il componente `Container` con capacità.
    std::vector<Entity> contents{};
    bool open = false;       // scrigno: lucchetto aperto e coperchio sollevato
    float openDeg = 0;       // animazione del coperchio (0..105°): stato visivo, avanza nel ciclo principale
    // Arredi e supporti (M4c, tranche 2). Campi "piatti" provvisori: con F1/F2 diventeranno i componenti Thermal, Anim, Mount.
    int fireState = 0;       // tavolo e rastrelliera: 0 integro, 1 in fiamme, 2 bruciato/crollato
    float fireT = 0;         // secondi di combustione trascorsi (a 4,5 s l'oggetto crolla)
    int slot = -1;           // oggetto appeso a un supporto (rastrelliera, ganci): posto che occupa; -1 = non appeso
    bool armsFree = false;   // scheletro: le braccia non sono più sostenute dal tavolo e oscillano liberamente
};

inline int contentIndex(const Entity& c, int def) {       // posizione di un oggetto di tipo `def` dentro `c`, -1 se non c'è
    for (int i = 0; i < (int)c.contents.size(); i++) if (c.contents[i].def == def) return i;
    return -1;
}
inline bool holds(const Entity& c, int def) { return contentIndex(c, def) >= 0; }
