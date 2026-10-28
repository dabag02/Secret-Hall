// Livello render — Scena: oggetti sciolti e trasportati, e disegno dell'intera sala (drawWorld).
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void drawLooseObj(const Entity& o) {
    PM _; glTranslatef(o.pos.x, o.pos.y, o.pos.z); glRotatef(o.yaw * 180.f / PI, 0, 1, 0);
    g_tintHot = o.hot > 0;                                          // pezzi roventi: colore pulsante
    switch (o.def) {
        case D_BUCKET: drawBucketBody(); break;
        case D_CHEST: drawChest(o); break;
        case D_HELM: case D_CUIRASS: case D_PAULDRON: case D_GREAVE: case D_VAMBRACE: case D_BOOT: drawArmorLying(o.def); break;
        case D_GEM: glTranslatef(0, .08f, 0); glRotatef(g_time * 40, 0, 1, 0); drawGemModel(); break;
        case D_COINS: drawCoinsModel(); break;
        case D_BOOK:
            drawBookModel();
            if (o.burn > 0) for (int k = 0; k < 3; k++) { PM f; glTranslatef(-.12f + k * .12f, .04f, (k - 1) * .04f); glScalef(1.5f, 1.5f, 1.5f); drawFlame(k * 1.3f); }
            break;
        case D_KEY: glTranslatef(0, .02f, 0); glRotatef(-90, 1, 0, 0); glScalef(.7f, .7f, .7f); col(.80f, .55f, .15f); drawKey(); break;
        case D_ASH: drawAshModel(); break;
        case D_SWORD: glTranslatef(0, .03f, 0); glRotatef(-90, 1, 0, 0); glTranslatef(0, -2.7f, -.54f); drawSword(); break;
        case D_SHIELD: glTranslatef(0, .03f, 0); glRotatef(-90, 1, 0, 0); glTranslatef(0, -2.0f, -.045f); drawShield(); break;
        case D_TORCH: {
            glTranslatef(0, .04f, 0);
            { PM b; glRotatef(-90, 0, 0, 1); drawTorchBody(o.lit, false); }
            if (o.lit) { glTranslatef(.40f, 0, 0); drawFlame(); }
        } break;
    }
    g_tintHot = false;
}

void drawLooseObjects() {
    for (auto& o : g_loose) drawLooseObj(o);
    if (g_carrying) drawLooseObj(g_carry);
}

void drawWorld() {
    drawFloor();
    drawCeiling();
    for (int i = 0; i < 4; i++) wallStones(W[i]);
    for (int i = 0; i < 4; i++) {
        PM _; wallFrame(W[i], DOOR_U[i]); drawDoor(); drawPainting(i);
        if (g_zapT > 0 && g_zapDoor == i) drawZap();
    }
    { PM _; wallFrame(W[2], 5.f); drawFireplace(); drawSwordHooks(); if (swordOnWall()) drawSword(); }
    { PM _; wallFrame(W[3], 5.f); drawShieldHooks(); if (shieldOnWall()) drawShield();
      if (rackState() == 2) drawRackRemains(); else drawRack();
      if (rackState() == 1) drawRackFire(); }
    if (tableState() == 2) drawTableRemains(); else drawTable();
    if (tableState() == 1) drawTableFire();
    drawChair(); drawSkeleton(); drawWallTorches(); drawLooseObjects();
}
