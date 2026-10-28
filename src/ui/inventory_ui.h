// Pannello dell'inventario: griglia 3x3, due mani, 10 slot di equipaggiamento attorno alla figura, statistiche, salute e pesi.
// Slot: 0..8 griglia, 9 = mano sinistra, 10 = mano destra, 11..20 = equipaggiamento (elmo, corazza, spalline, bracciali, gambali, stivali).
#pragma once
#include "items.h"
#include "ui/text.h"

// slot: 0..8 griglia, 9 = mano sinistra, 10 = mano destra
struct InvLayout { float slot, gap, pad, titleH, px, py, pw, ph, hx, hy0, hy1, gx, gy, fx, fy, fw, fh; };
extern float g_figAngle;                                  // angolo di rotazione della figura (rad)

InvLayout invLayout(int w, int h);                        // posizione di tutti gli elementi per una finestra w x h
void slotRect(const InvLayout& L, int i, float& x, float& y);
int invHit(const InvLayout& L, float mx, float my);       // slot sotto il mouse (-1 nessuno)
bool figHit(const InvLayout& L, float mx, float my);      // il mouse è sulla figura?
void drawItemIcon(const Entity& it, float cx, float cy, float s);   // icona di un oggetto in uno slot di lato s
void drawInventory(TextCache& tc, int w, int h, int hover);       // pannello completo (sfondo, slot, figura, statistiche, pesi)
