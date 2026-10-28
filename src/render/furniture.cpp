// Livello render — Arredi: tavolo (integro, in fiamme, bruciato), sedia, scrigno.
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void drawTable() {
    col(.92f, .56f, .22f);
    boxAt(0, .84f, 0, 3.0f, .12f, 1.3f);                    // piano (faccia superiore y=0.90)
    boxAt(0, .70f, 0, 2.7f, .12f, 1.0f);                    // cinta
    strip({{-1.35f, .901f, -.55f}, {1.35f, .901f, -.55f}, {1.35f, .901f, .55f}, {-1.35f, .901f, .55f}}, true);
    for (int s = -1; s <= 1; s += 2)                        // intaglio a archetti sulla cinta
        for (int i = 0; i < 13; i++)
            arc({-1.2f + i * .2f, .66f, s * .501f}, {.07f, 0, 0}, {0, -.05f, 0}, 0, PI, 6);
    for (int i = 0; i < 30; i++) {                          // greca a zig-zag sul bordo
        float x = -1.45f + i * .1f, y = (i & 1) ? .80f : .84f;
        line(x, y, .651f, x + .1f, (i & 1) ? .84f : .80f, .651f);
        line(x, y, -.651f, x + .1f, (i & 1) ? .84f : .80f, -.651f);
    }
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {               // gambe tornite
            PM _; glTranslatef(sx * 1.3f, 0, sz * .45f);
            cone(.07f, .05f, .22f, 8);
            { PM b; glTranslatef(0, .34f, 0); sphere(.085f, .10f, .085f, 3, 6); }
            { PM c; glTranslatef(0, .46f, 0); cone(.05f, .06f, .26f, 8); }
            boxAt(0, .70f, 0, .14f, .12f, .14f);
        }
    boxAt(0, .20f, .45f, 2.6f, .05f, .05f);                 // traverse
    boxAt(0, .20f, -.45f, 2.6f, .05f, .05f);
    boxAt(-1.3f, .20f, 0, .05f, .05f, .9f);
    boxAt(1.3f, .20f, 0, .05f, .05f, .9f);
}

void drawChair() {
    col(.80f, .48f, .18f);
    boxAt(2.05f, .50f, 0, .5f, .05f, .5f);                  // seduta
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            boxAt(2.05f + sx * .22f, .25f, sz * .22f, .05f, .5f, .05f);
    for (int sz = -1; sz <= 1; sz += 2) {
        boxAt(2.30f, .98f, sz * .22f, .05f, .95f, .05f);    // montanti dello schienale
        { PM _; glTranslatef(2.30f, 1.48f, sz * .22f); sphere(.04f, .04f, .04f, 2, 6); }
    }
    boxAt(2.30f, 1.38f, 0, .05f, .10f, .5f);                // traversa alta
    boxAt(2.30f, .62f, 0, .05f, .05f, .5f);                 // traversa bassa
    for (int i = 0; i < 5; i++) line(2.30f, .66f, -.2f + i * .1f, 2.30f, 1.33f, -.2f + i * .1f);
    boxAt(2.05f, .25f, 0, .4f, .04f, .04f);                 // stretcher
}

void drawChest(const Entity& chest) {
    PM _;
    col(.80f, .45f, .16f);
    boxAt(0, .14f, 0, .8f, .28f, .5f);                      // corpo
    col(.62f, .67f, .78f);                                  // fasce metalliche
    for (float x : {-.28f, .28f}) {
        line(x, 0, .25f, x, .28f, .25f); line(x, .28f, -.25f, x, .28f, .25f); line(x, 0, -.25f, x, .28f, -.25f);
    }
    rect(-.05f, .10f, .05f, .24f, .252f);                   // piastra della serratura
    {   // coperchio a botte, incernierato sul bordo posteriore
        PM l; glTranslatef(0, .28f, -.25f); glRotatef(-chest.openDeg, 1, 0, 0);
        col(.80f, .45f, .16f);
        const int N = 8;
        for (int j = 0; j <= 4; j++) {
            float x = -.4f + j * .2f;
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= N; i++) { float u = (float)i / N; glVertex3f(x, .14f * sinf(PI * u), .5f * u); }
            glEnd();
        }
        for (int i = 0; i <= N; i += 2) {
            float u = (float)i / N;
            line(-.4f, .14f * sinf(PI * u), .5f * u, .4f, .14f * sinf(PI * u), .5f * u);
        }
        col(.62f, .67f, .78f);
        for (float x : {-.28f, .28f}) {
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= N; i++) { float u = (float)i / N; glVertex3f(x, .145f * sinf(PI * u), .5f * u); }
            glEnd();
        }
    }
    if (!chest.open) {                                      // lucchetto
        col(.96f, .72f, .22f);
        boxAt(0, .12f, .27f, .07f, .09f, .03f);
        arc({0, .165f, .27f}, {.025f, 0, 0}, {0, .04f, 0}, 0, PI, 8);
        ringXY({0, .12f, .286f}, .008f, .008f, 6);
    } else {                                                // contenuto: gemma e monete (se ancora dentro)
        for (const Entity& in : chest.contents) {
            if (in.def == D_GEM) { PM g; glTranslatef(0, .22f, 0); glRotatef(g_time * 60, 0, 1, 0); drawGemModel(); }
            else if (in.def == D_COINS) { PM c; glTranslatef(.1f, .02f, .05f); drawCoinsModel(); }
        }
    }
}

void drawTableFire() {
    for (int k = 0; k < 6; k++) {
        PM _; glTranslatef(-1.25f + k * .5f, .93f, (k & 1) ? .22f : -.22f); glScalef(2.2f, 2.2f, 2.2f); drawFlame(k * 1.3f);
    }
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) { PM _; glTranslatef(sx * 1.3f, .30f, sz * .45f); glScalef(1.6f, 1.6f, 1.6f); drawFlame((sx + 2 * sz) * 1.1f); }
}

void drawTableRemains() {               // tavolo carbonizzato: monconi, assi cadute, cenere
    col(.30f, .26f, .23f);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) { PM _; glTranslatef(sx * 1.3f, 0, sz * .45f); glRotatef(sx * sz * 8.f, 0, 0, 1); cone(.06f, .045f, sx * sz > 0 ? .21f : .16f, 8); }
    { PM _; glTranslatef(-.3f, .025f, .15f); glRotatef(18, 0, 1, 0); box(1.3f, .05f, .22f); }
    { PM _; glTranslatef(.4f, .025f, -.2f); glRotatef(-25, 0, 1, 0); box(1.1f, .05f, .20f); }
    { PM _; glTranslatef(0, .06f, 0); glRotatef(70, 0, 1, 0); box(.9f, .05f, .18f); }
    col(.42f, .40f, .38f);
    ringXZ({0, .01f, 0}, .8f, .5f, 16); ringXZ({0, .03f, 0}, .55f, .32f, 14); ringXZ({0, .05f, 0}, .3f, .18f, 12);
    col(1.f, .40f, .12f); ringXZ({.1f, .06f, .05f}, .08f, .06f, 8); ringXZ({-.3f, .06f, -.1f}, .06f, .05f, 8);   // braci
}
