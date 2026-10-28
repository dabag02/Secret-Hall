// Livello render — Rastrelliera dell'armatura: integra, in fiamme e carbonizzata.
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void drawRack() {                      // locale: x lungo il muro, z verso la stanza
    col(.62f, .38f, .16f);                   // struttura di legno
    boxAt(-1.15f, .75f, .25f, .07f, 1.5f, .07f);
    boxAt(1.15f, .75f, .25f, .07f, 1.5f, .07f);
    boxAt(-1.15f, .035f, .25f, .07f, .07f, .5f);
    boxAt(1.15f, .035f, .25f, .07f, .07f, .5f);
    boxAt(0, 1.45f, .25f, 2.3f, .05f, .06f);   // barra superiore
    boxAt(0, .80f, .25f, 2.3f, .04f, .45f);    // ripiano medio
    boxAt(0, .30f, .25f, 2.3f, .04f, .45f);    // ripiano basso
    line(-1.15f, 1.5f, .25f, -1.15f, 1.5f, 0); line(1.15f, 1.5f, .25f, 1.15f, 1.5f, 0);

    for (int sd = -1; sd <= 1; sd += 2) {                          // i pezzi in vista sono quelli ancora sulla rastrelliera
        int a = sd < 0 ? 0 : 1;
        col(.62f, .67f, .78f); line(sd * .58f, 1.35f, .25f, sd * .58f, 1.45f, .25f);   // gancio delle spalline
        if (rackHas(1 + a)) { PM _; glTranslatef(sd * .58f, 1.25f, .25f); drawPauldronModel(); }
        if (rackHas(3 + a)) { PM _; glTranslatef(sd * .70f, .90f, .25f); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.225f, 0); drawGreaveModel(); }
        if (rackHas(5 + a)) { PM _; glTranslatef(sd * .22f, .88f, .25f); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.15f, 0); drawVambraceModel(); }
        if (rackHas(7 + a)) { PM _; glTranslatef(sd * .65f, .32f, .18f); drawBootModel(); }
    }
    if (rackHas(0)) { PM _; glTranslatef(0, .95f, .25f); drawCuirassModel(); }
}

void drawRackFire() {                   // nel riferimento locale della parete ovest
    for (int i = 0; i < 5; i++) { PM _; glTranslatef(-1.0f + i * .5f, .20f + (i & 1) * .75f, .25f); glScalef(2.2f, 2.2f, 2.2f); drawFlame(i * 1.1f); }
    for (int sd = -1; sd <= 1; sd += 2) { PM _; glTranslatef(sd * 1.15f, .95f, .25f); glScalef(2.6f, 2.6f, 2.6f); drawFlame(sd * 2.3f); }
}

void drawRackRemains() {                // rastrelliera carbonizzata: monconi, assi cadute, cenere
    col(.30f, .26f, .23f);
    for (int sd = -1; sd <= 1; sd += 2) { PM _; glTranslatef(sd * 1.15f, 0, .25f); glRotatef(-sd * 6.f, 0, 0, 1); cone(.045f, .03f, .42f, 8); }
    { PM _; glTranslatef(-.2f, .04f, .5f); glRotatef(15, 0, 1, 0); box(2.2f, .06f, .07f); }
    { PM _; glTranslatef(.3f, .035f, .8f); glRotatef(-30, 0, 1, 0); box(1.2f, .05f, .45f); }
    { PM _; glTranslatef(0, .05f, .35f); glRotatef(80, 0, 1, 0); box(1.0f, .05f, .06f); }
    col(.42f, .40f, .38f); ringXZ({0, .01f, .4f}, .9f, .5f, 16); ringXZ({0, .03f, .4f}, .6f, .3f, 14);
    col(1.f, .40f, .12f); ringXZ({.2f, .06f, .5f}, .08f, .06f, 8);
}
