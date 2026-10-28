// Uomo in wireframe (per il pannello dell'inventario): tunica con cintura, calzamaglia, sandali, armatura indossata.
#pragma once
#include "core/types.h"

void limb(V a, V b, float r0, float r1, int n = 8);      // tronco di cono tra due punti qualsiasi
void drawMan();                                           // alto ~1,75 u, piedi nell'origine, fronte verso +z
