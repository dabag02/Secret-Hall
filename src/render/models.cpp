// Livello render — Modelli di oggetti piccoli (origine locale): elmo e pezzi d'armatura, gemma, monete, libro, cenere, chiave, fiamma, torcia, secchio.
#include "game/defs.h"
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void drawHelm(float r) {               // fronte verso +z
    dome(r, r * 1.1f, r, 0, PI / 2, 3, 8);
    { PM _; glTranslatef(0, -.75f * r, 0); cone(r * .95f, r, .75f * r, 12, 12); }
    auto zf = [&](float x) { return sqrtf(std::max(0.f, r * r - x * x)) * 1.03f; };
    strip({{-.55f * r, -.20f * r, zf(.55f * r)}, {.55f * r, -.20f * r, zf(.55f * r)},
           {.55f * r, -.05f * r, zf(.55f * r)}, {-.55f * r, -.05f * r, zf(.55f * r)}}, true);   // feritoia
    line(0, .35f * r, r * 1.03f, 0, -.70f * r, r * 1.03f);                                       // nasale
    for (int i = 0; i < 4; i++) { float y = -.35f * r - .1f * r * i; line(.2f * r, y, zf(.2f * r), .5f * r, y, zf(.5f * r)); }
}

void drawCuirassModel() {              // origine alla base (appesa alla barra)
    col(.70f, .78f, .92f);
    PM _; glScalef(1, 1, .7f);
    cone(.17f, .22f, .5f, 12, 12);
    ringXZ({0, .5f, 0}, .08f, .08f, 10);
    line(0, .02f, .22f, 0, .5f, .17f);
    ringYZ({.20f, .36f, 0}, .06f, .06f, 8); ringYZ({-.20f, .36f, 0}, .06f, .06f, 8);
}

void drawPauldronModel() { col(.70f, .78f, .92f); dome(.14f, .10f, .14f, 0, PI / 2, 3, 8); }

void drawGreaveModel() { col(.70f, .78f, .92f); cone(.07f, .055f, .45f, 8); }       // lungo +Y

void drawVambraceModel() { col(.70f, .78f, .92f); cone(.05f, .04f, .30f, 8); }

void drawBootModel() { col(.70f, .78f, .92f); cone(.065f, .055f, .30f, 8, 8); boxAt(0, .04f, .10f, .10f, .08f, .26f); }

void drawGemModel() {
    col(.4f, 1.f, 1.f);
    V t{0, .07f, 0}, b{0, -.07f, 0}, p[4] = {{.05f, 0, 0}, {0, 0, .05f}, {-.05f, 0, 0}, {0, 0, -.05f}};
    for (int i = 0; i < 4; i++) { line(t, p[i]); line(b, p[i]); line(p[i], p[(i + 1) % 4]); }
}

void drawBookModel() {                  // libro aperto appoggiato, origine al centro della base
    col(.55f, .18f, .14f); boxAt(0, .008f, 0, .40f, .016f, .28f);
    col(.95f, .92f, .80f);
    { PM _; glTranslatef(-.095f, .026f, 0); glRotatef(4, 0, 0, 1); box(.18f, .02f, .25f); }
    { PM _; glTranslatef(.095f, .026f, 0); glRotatef(-4, 0, 0, 1); box(.18f, .02f, .25f); }
    col(.45f, .40f, .30f);
    for (int k = 0; k < 6; k++) { float z = -.09f + k * .036f; line(-.17f, .038f, z, -.03f, .038f, z); line(.03f, .038f, z, .17f, .038f, z); }
    line(0, .02f, -.125f, 0, .02f, .125f);
}

void drawAshModel() {                   // cenere tossica (verdastra)
    col(.50f, .62f, .40f);
    ringXZ({0, .01f, 0}, .16f, .12f, 12); ringXZ({0, .03f, 0}, .10f, .08f, 10); ringXZ({0, .05f, 0}, .04f, .03f, 8);
    col(.65f, .75f, .50f);
    for (int k = 0; k < 6; k++) { float t = k * 1.047f; line(.16f * cosf(t), .01f, .12f * sinf(t), .04f * cosf(t + .4f), .05f, .03f * sinf(t + .4f)); }
}

void drawCoinsModel() {                 // due piccole pile di monete
    col(1.f, .85f, .30f);
    for (int k = 0; k < 3; k++) { PM _; glTranslatef(0, .012f + .022f * k, 0); cone(.06f, .06f, .018f, 12, 6); }
    for (int k = 0; k < 2; k++) { PM _; glTranslatef(.12f, .012f + .022f * k, .06f); cone(.06f, .06f, .018f, 12, 6); }
}

void drawFlame(float phase) {          // origine = base della fiamma, asse +Y; non subisce l'illuminazione
    GLboolean lt = glIsEnabled(GL_LIGHTING); glDisable(GL_LIGHTING);
    float gt = g_time + phase;
    float f1 = 1 + .18f * sinf(gt * 17) + .10f * sinf(gt * 29 + 1), sway = .018f * sinf(gt * 11);
    const int N = 8;
    for (int layer = 0; layer < 2; layer++) {
        float sc = layer ? .55f : 1.f, h = .30f * f1 * sc, r = .06f * sc;
        if (layer) col(1.f, .93f, .45f); else col(1.f, .52f, .12f);
        for (int pl = 0; pl < 2; pl++) {
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i <= N; i++) {
                float t = (float)i / N, x = -r * sinf(PI * powf(t, .6f)) + sway * t, y = h * t;
                if (pl) glVertex3f(0, y, x); else glVertex3f(x, y, 0);
            }
            for (int i = N - 1; i >= 1; i--) {
                float t = (float)i / N, x = r * sinf(PI * powf(t, .6f)) + sway * t, y = h * t;
                if (pl) glVertex3f(0, y, x); else glVertex3f(x, y, 0);
            }
            glEnd();
        }
    }
    if (lt) glEnable(GL_LIGHTING);
}

void drawTorchBody(bool lit, bool withFlame) {         // origine = punto di appoggio, asse +Y (manico -0,30 .. testa +0,40)
    col(.66f, .40f, .17f);
    { PM _; glTranslatef(0, -.30f, 0); cone(.022f, .030f, .55f, 8, 8); }
    col(.62f, .67f, .78f);
    ringXZ({0, -.02f, 0}, .034f, .034f, 8); ringXZ({0, .18f, 0}, .034f, .034f, 8);
    if (lit) col(.60f, .38f, .20f); else col(.36f, .30f, .27f);
    {   // testa di stoppa
        PM _; glTranslatef(0, .25f, 0); cone(.032f, .062f, .15f, 10, 10);
        for (int k = 1; k < 4; k++) ringXZ({0, .15f * k / 4, 0}, .032f + .03f * k / 4, .032f + .03f * k / 4, 10);
    }
    if (lit && withFlame) { PM _; glTranslatef(0, .40f, 0); drawFlame(); }
}

void drawKey() {                       // lunghezza ~0.42 unità, nel piano XY
    ringXY({0, 0, 0}, .06f, .06f, 14); ringXY({0, 0, 0}, .03f, .03f, 10);
    line(.06f, 0, 0, .42f, 0, 0);
    strip({{.30f, 0, 0}, {.30f, -.09f, 0}, {.36f, -.09f, 0}, {.36f, 0, 0}});
    strip({{.42f, 0, 0}, {.42f, -.06f, 0}, {.40f, -.06f, 0}});
}

void drawBucketBody() {
    PM _;
    col(.78f, .46f, .18f);
    cone(.17f, .22f, .4f, 12, 12);                          // doghe
    col(.62f, .67f, .78f);
    ringXZ({0, .08f, 0}, .187f, .187f, 16); ringXZ({0, .30f, 0}, .213f, .213f, 16);   // cerchi di ferro
    arc({0, .40f, 0}, {.22f, 0, 0}, {0, .22f, 0}, 0, PI, 12);                          // manico
    col(.58f, .58f, .60f);                                  // cenere spenta
    ringXZ({0, .34f, 0}, .20f, .20f, 16); ringXZ({0, .365f, 0}, .14f, .14f, 12);
    ringXZ({0, .385f, 0}, .08f, .08f, 10); ringXZ({0, .395f, 0}, .035f, .035f, 8);
    for (int i = 0; i < 9; i++) {
        float t = 2 * PI * i / 9, j = (i % 2) ? .02f : -.01f;
        line((.20f) * cosf(t), .34f, .20f * sinf(t), (.04f + j) * cosf(t + .3f), .395f, (.04f + j) * sinf(t + .3f));
    }
    // la chiave NON si disegna finché giace sotto la cenere: la si scopre solo frugando (E)
}

void drawArmorLying(int t) {            // pezzo d'armatura appoggiato a terra
    switch (t) {
        case D_HELM: glTranslatef(0, .14f, 0); glRotatef(82, 0, 0, 1); glRotatef(35, 0, 1, 0); col(.70f, .78f, .92f); drawHelm(.13f); break;
        case D_CUIRASS: glTranslatef(0, .20f, 0); glRotatef(90, 0, 0, 1); glTranslatef(0, -.25f, 0); drawCuirassModel(); break;
        case D_PAULDRON: drawPauldronModel(); break;
        case D_GREAVE: glTranslatef(0, .07f, 0); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.225f, 0); drawGreaveModel(); break;
        case D_VAMBRACE: glTranslatef(0, .05f, 0); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.15f, 0); drawVambraceModel(); break;
        case D_BOOT: glTranslatef(0, .065f, 0); glRotatef(-90, 0, 0, 1); drawBootModel(); break;
    }
}

void drawItemModel(int type) {          // modelli centrati e normalizzati (~0,45 u) per icone e mano
    PM _;
    switch (type) {
        case D_GEM: glScalef(3.5f, 3.5f, 3.5f); drawGemModel(); break;
        case D_COINS: glScalef(1.7f, 1.7f, 1.7f); glTranslatef(-.06f, -.02f, -.03f); drawCoinsModel(); break;      // le monete: icona più piccola
        case D_BOOK: glScalef(1.05f, 1.05f, 1.05f); glTranslatef(0, -.02f, 0); drawBookModel(); break;
        case D_HELM: glScalef(1.7f, 1.7f, 1.7f); col(.70f, .78f, .92f); drawHelm(.13f); break;
        case D_CUIRASS: glScalef(.9f, .9f, .9f); glTranslatef(0, -.25f, 0); drawCuirassModel(); break;
        case D_PAULDRON: glScalef(1.6f, 1.6f, 1.6f); glTranslatef(0, -.04f, 0); drawPauldronModel(); break;
        case D_GREAVE: glTranslatef(0, -.225f, 0); drawGreaveModel(); break;
        case D_VAMBRACE: glScalef(1.5f, 1.5f, 1.5f); glTranslatef(0, -.15f, 0); drawVambraceModel(); break;
        case D_BOOT: glScalef(1.4f, 1.4f, 1.4f); glTranslatef(0, -.15f, 0); drawBootModel(); break;
    }
}
