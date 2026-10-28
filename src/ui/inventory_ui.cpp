// Interfaccia — pannello dell'inventario.
#include "ui/inventory_ui.h"
#include "ui/figure.h"
#include "items.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "render/render.h"
#include "ui/draw2d.h"
#include "ui/text.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

float g_figAngle = .5f;

InvLayout invLayout(int w, int h) {
    InvLayout L; L.slot = 88; L.gap = 10; L.pad = 26; L.titleH = 34;
    float grid = 3 * L.slot + 2 * L.gap, handGap = 40, figGap = 30; L.fw = 210; L.fh = grid;
    L.pw = L.pad * 2 + L.slot + handGap + grid + figGap + L.fw; L.ph = L.pad * 2 + L.titleH + grid + 182;   // 182 = statistiche sotto la figura e righe del peso
    L.px = (w - L.pw) / 2; L.py = (h - L.ph) / 2 - 36;
    L.hx = L.px + L.pad; L.gx = L.hx + L.slot + handGap; L.gy = L.py + L.pad + L.titleH;
    L.hy0 = L.gy + 36; L.hy1 = L.hy0 + L.slot + 44;
    L.fx = L.gx + grid + figGap; L.fy = L.gy;
    return L;
}

void slotRect(const InvLayout& L, int i, float& x, float& y) {
    if (i == 9) { x = L.hx; y = L.hy0; return; }
    if (i == 10) { x = L.hx; y = L.hy1; return; }
    if (i >= 11) {                                              // slot di equipaggiamento ai lati della figura
        int k = i - 11; float sz = 32;
        x = (k & 1) ? L.fx + L.fw - 6 - sz : L.fx + 6; y = L.fy + 8 + (k / 2) * ((L.fh - 16 - sz) / 4);
        return;
    }
    x = L.gx + (i % 3) * (L.slot + L.gap); y = L.gy + (i / 3) * (L.slot + L.gap);
}

int invHit(const InvLayout& L, float mx, float my) {
    for (int i = 0; i < 21; i++) { float x, y, sz = i >= 11 ? 32.f : L.slot; slotRect(L, i, x, y); if (mx >= x && mx <= x + sz && my >= y && my <= y + sz) return i; }
    return -1;
}

bool figHit(const InvLayout& L, float mx, float my) { return mx >= L.fx && mx <= L.fx + L.fw && my >= L.fy && my <= L.fy + L.fh; }

void drawItemIcon(const Entity& it, float cx, float cy, float s) {
    if (!it.def) return;
    PM _; glTranslatef(cx, cy, 0);
    switch (it.def) {
        case D_KEY:
            glRotatef(35, 0, 0, 1); glScalef(s * 1.2f, -s * 1.2f, s); glTranslatef(-.24f, 0, 0);
            col(.95f, .70f, .20f); drawKey(); break;
        case D_TORCH:
            glRotatef(22, 0, 0, 1); glScalef(s * .80f, -s * .80f, s * .80f); glRotatef(28, 1, 0, 0); glRotatef(35, 0, 1, 0); glTranslatef(0, -.22f, 0);
            drawTorchBody(it.lit); break;
        case D_SWORD:
            glRotatef(-42, 0, 0, 1); glScalef(s * .40f, -s * .40f, s * .40f); glTranslatef(0, -2.7f, -.54f);
            drawSword(); break;
        case D_SHIELD:
            glScalef(s * .55f, -s * .55f, s * .55f); glRotatef(20, 0, 1, 0); glTranslatef(0, -2.0f, -.045f);
            drawShield(); break;
        case D_BUCKET:
            glScalef(s * .95f, -s * .95f, s * .95f); glRotatef(22, 1, 0, 0); glTranslatef(0, -.30f, 0);
            drawBucketBody(); break;
        case D_GEM: case D_COINS: case D_BOOK: case D_HELM: case D_CUIRASS: case D_PAULDRON: case D_GREAVE: case D_VAMBRACE: case D_BOOT:
            glScalef(s * 1.5f, -s * 1.5f, s * 1.5f); glRotatef(22, 1, 0, 0); glRotatef(30, 0, 1, 0); drawItemModel(it.def); break;
    }
}

void drawInventory(TextCache& tc, int w, int h, int hover) {
    fillRect(0, 0, (float)w, (float)h, 0, 0, 0, .50f);
    InvLayout L = invLayout(w, h);
    fillRect(L.px, L.py, L.pw, L.ph, .03f, .03f, .05f, .88f);
    strokeRect(L.px, L.py, L.pw, L.ph, .8f, .8f, .9f, .5f);
    tc.draw("Mani", L.hx, L.py + L.pad - 2);
    tc.draw("Inventario", L.gx, L.py + L.pad - 2);
    tc.draw("Personaggio", L.fx, L.py + L.pad - 2);
    tc.draw("Sinistra", L.hx, L.hy0 - 27); tc.draw("Destra", L.hx, L.hy1 - 27);
    for (int i = 0; i < 11; i++) {
        float x, y; slotRect(L, i, x, y);
        fillRect(x, y, L.slot, L.slot, 0, 0, 0, .45f);
        if (i == hover) strokeRect(x, y, L.slot, L.slot, 1.f, .85f, .2f, 1.f);
        else if (i >= 9) strokeRect(x, y, L.slot, L.slot, .95f, .78f, .3f, .9f);
        else strokeRect(x, y, L.slot, L.slot, .55f, .60f, .70f, .8f);
        drawItemIcon(i >= 9 ? g_hand[i - 9] : g_inv[i], x + L.slot / 2, y + L.slot / 2, L.slot);
        if (i < 9) tc.draw(std::to_string(i + 1), x + 7, y + 3);
    }
    // figura: ruotabile trascinando (o con le frecce)
    fillRect(L.fx, L.fy, L.fw, L.fh, 0, 0, 0, .45f);
    strokeRect(L.fx, L.fy, L.fw, L.fh, .55f, .60f, .70f, .8f);
    {
        PM _; float sc = (L.fh - 74) / 1.78f;
        glTranslatef(L.fx + L.fw / 2, L.fy + L.fh - 40, 0); glScalef(sc, -sc, sc);
        glRotatef(10, 1, 0, 0); glRotatef(g_figAngle * 180.f / PI, 0, 1, 0);
        drawMan();
    }
    tc.draw("ruotabile", L.fx + 68, L.fy + 8);
    for (int i = 0; i < 10; i++) {                                  // slot di equipaggiamento
        float x, y; slotRect(L, 11 + i, x, y);
        fillRect(x, y, 32, 32, 0, 0, 0, .45f);
        if (hover == 11 + i) strokeRect(x, y, 32, 32, 1.f, .85f, .2f, 1.f);
        else strokeRect(x, y, 32, 32, g_equip[i] ? .95f : .45f, g_equip[i] ? .78f : .50f, g_equip[i] ? .30f : .60f, .9f);
        if (g_equip[i]) drawItemIcon(Entity{SLOT_TYPE[i], false}, x + 16, y + 16, 32);
    }
    // statistiche del personaggio (con bonus/malus dell'armatura), sotto la figura
    float sy = L.fy + L.fh + 14; int na = ARMOR_BONUS_PER_PIECE ? armorCount() : (armorCount() > 0 ? 1 : 0);
    for (int i = 0; i < 5; i++) {
        float y = sy + i * 26;
        int eff = std::max(0, g_stats[i] + STAT_MOD[i] * na), dlt = STAT_MOD[i] * na;
        std::string v = std::to_string(eff) + (dlt ? std::string(" (") + (dlt > 0 ? "+" : "") + std::to_string(dlt) + ")" : "");
        tc.draw(STAT_NAMES[i], L.fx + 2, y);
        tc.draw(v, L.fx + 90, y);
        float bx = L.fx + 158, bw = L.fw - 158 - 4, f = std::min(1.f, eff / 12.f);
        fillRect(bx, y + 8, bw * f, 11, i == 4 ? .90f : .95f, i == 4 ? .30f : .70f, i == 4 ? .30f : .25f, .85f);
        strokeRect(bx, y + 8, bw, 11, .55f, .60f, .70f, .8f);
    }
    {   // salute corrente
        float y = sy + 5 * 26, bx = L.fx + 158, bw = L.fw - 158 - 4, f = std::min(1.f, (float)g_hp / hpMax());
        tc.draw("Salute", L.fx + 2, y); tc.draw(std::to_string(g_hp) + " / " + std::to_string(hpMax()), L.fx + 90, y);
        fillRect(bx, y + 8, bw * f, 11, .35f, .80f, .35f, .85f); strokeRect(bx, y + 8, bw, 11, .55f, .60f, .70f, .8f);
    }
    {   // peso: il peso di ciò che si indossa e si tiene in mano riduce la capacità dell'inventario
        float gy = L.gy + 3 * L.slot + 2 * L.gap + 14, cap = MAX_KG - wornKg() - handKg();
        tc.draw("Peso trasportato: " + fmtKg(totalKg()) + " / " + fmtKg(MAX_KG) + " kg", L.hx, gy);
        tc.draw("Inventario: " + fmtKg(invKg()) + " kg su " + fmtKg(std::max(0.f, cap)) + " kg di capacità", L.hx, gy + 26);
        tc.draw("In mano " + fmtKg(handKg()) + " kg · indossato " + fmtKg(wornKg()) + " kg", L.hx, gy + 52);
    }
}
