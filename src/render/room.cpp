// Livello render — Stanza: pareti a blocchi, pavimento, travi, porte, quadri e scarica delle porte magiche.
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void wallFrame(const Wall& w, float u) {
    float m[16] = {w.ux, 0, w.uz, 0,  0, 1, 0, 0,  w.nx, 0, w.nz, 0,
                   w.ox + w.ux * u, 0, w.oz + w.uz * u, 1};
    glMultMatrixf(m);
}

void wallStones(const Wall& w) {
    PM _; wallFrame(w, 0);
    col(.34f, .40f, .48f);
    std::vector<Hole> hs = w.holes;
    std::sort(hs.begin(), hs.end(), [](const Hole& a, const Hole& b) { return a.u0 < b.u0; });
    for (int k = 0; k <= 8; k++) {            // filari orizzontali
        float v = .5f * k, cur = 0;
        for (auto& h : hs)
            if (v < h.v1 - 1e-3f) {
                if (h.u0 > cur) line(cur, v, 0, h.u0, v, 0);
                cur = std::max(cur, h.u1);
            }
        if (cur < WL) line(cur, v, 0, WL, v, 0);
    }
    for (int k = 0; k < 8; k++) {             // giunti verticali sfalsati
        float v0 = .5f * k, v1 = v0 + .5f, off = (k & 1) ? .5f : 0.f;
        for (int j = 0; j <= 10; j++) {
            float u = j + off;
            if (u < .01f || u > WL - .01f) continue;
            bool in = false;
            for (auto& h : hs) if (u > h.u0 && u < h.u1 && v0 < h.v1 - 1e-3f) in = true;
            if (!in) line(u, v0, 0, u, v1, 0);
        }
    }
}

void drawFloor() {
    col(.20f, .25f, .33f);
    for (int r = 0; r < 10; r++) {            // lastre sfalsate
        float z0 = -5.f + r, off = (r & 1) ? .5f : 0.f;
        line(-5, 0, z0, 5, 0, z0);
        for (int j = 0; j <= 10; j++) {
            float x = -5.f + j + off;
            if (x > -4.99f && x < 4.99f) line(x, 0, z0, x, 0, z0 + 1);
        }
    }
    line(-5, 0, 5, 5, 0, 5);
}

void drawCeiling() {
    col(.55f, .33f, .14f);                    // travi di quercia
    for (int i = 0; i < 4; i++) {
        float x = -3.75f + i * 2.5f;
        boxAt(x, WH - .15f, 0, .3f, .3f, 10.f);
    }
}

void drawDoor() {
    col(.55f, .60f, .68f);                    // stipite in pietra
    strip({{-.95f, 0, .02f}, {-.95f, 2.8f, .02f}, {.95f, 2.8f, .02f}, {.95f, 0, .02f}});
    line(-.8f, 2.6f, .02f, -.8f, 2.8f, .02f);
    col(.92f, .56f, .22f);                    // battente di legno
    strip({{-.8f, 0, .13f}, {-.8f, 2.6f, .13f}, {.8f, 2.6f, .13f}, {.8f, 0, .13f}});
    for (int k = 1; k < 8; k++) { float x = -.8f + .2f * k; line(x, 0, .13f, x, 2.6f, .13f); }
    for (int s = -1; s <= 1; s += 2) {        // spessore
        line(s * .8f, 2.6f, .03f, s * .8f, 2.6f, .13f);
        line(s * .8f, 0, .03f, s * .8f, 0, .13f);
    }
    line(-.8f, 2.6f, .03f, .8f, 2.6f, .03f);
    col(.62f, .67f, .78f);                    // fasce di ferro e borchie
    for (float y : {.45f, 1.3f, 2.15f}) {
        line(-.8f, y - .07f, .135f, .8f, y - .07f, .135f);
        line(-.8f, y + .07f, .135f, .8f, y + .07f, .135f);
        for (int k = 0; k <= 8; k++) { float x = -.8f + .2f * k; line(x, y - .03f, .14f, x, y + .03f, .14f); }
    }
    ringXY({.55f, 1.15f, .16f}, .08f, .08f, 12);   // anello-maniglia
    rect(.5f, 1.2f, .6f, 1.3f, .14f);
    ringXY({.55f, .95f, .14f}, .02f, .02f, 8);      // serratura
}

void drawPainting(int variant) {
    col(.96f, .78f, .30f);                    // cornice dorata
    rect(-.62f, 2.93f, .62f, 3.88f, .05f);
    rect(-.55f, 3.00f, .55f, 3.81f, .04f);
    for (float x : {-.62f, .62f}) { line(x, 2.93f, 0, x, 2.93f, .05f); line(x, 3.88f, 0, x, 3.88f, .05f); }
    rect(-.22f, 2.86f, .22f, 2.92f, .03f);    // targhetta
    col(.78f, .82f, .92f);                    // ritratto
    const float z = .04f;
    ringXY({0, 3.45f, z}, .10f, .13f, 14);
    line(-.04f, 3.33f, z, -.05f, 3.24f, z); line(.04f, 3.33f, z, .05f, 3.24f, z);
    strip({{-.05f, 3.24f, z}, {-.38f, 3.10f, z}, {-.45f, 3.00f, z}});
    strip({{.05f, 3.24f, z}, {.38f, 3.10f, z}, {.45f, 3.00f, z}});
    strip({{-.05f, 3.24f, z}, {0, 3.10f, z}, {.05f, 3.24f, z}});
    line(-.06f, 3.48f, z, -.02f, 3.48f, z); line(.02f, 3.48f, z, .06f, 3.48f, z);
    line(0, 3.46f, z, 0, 3.40f, z); line(-.03f, 3.37f, z, .03f, 3.37f, z);
    switch (variant) {
        case 0:  // corona
            strip({{-.10f, 3.56f, z}, {-.10f, 3.70f, z}, {-.05f, 3.63f, z}, {0, 3.72f, z},
                   {.05f, 3.63f, z}, {.10f, 3.70f, z}, {.10f, 3.56f, z}});
            break;
        case 1:  // elmo
            arc({0, 3.50f, z}, {.13f, 0, 0}, {0, .17f, 0}, 0, PI, 12);
            line(-.14f, 3.50f, z, .14f, 3.50f, z); line(0, 3.50f, z, 0, 3.40f, z);
            break;
        case 2:  // barba
            strip({{-.10f, 3.42f, z}, {-.08f, 3.25f, z}, {0, 3.19f, z}, {.08f, 3.25f, z}, {.10f, 3.42f, z}});
            break;
        default: // cappello a tesa larga
            ringXY({0, 3.58f, z}, .24f, .03f, 14);
            arc({0, 3.58f, z}, {.12f, 0, 0}, {0, .12f, 0}, 0, PI, 10);
            break;
    }
}

void drawZap() {                        // scarica della porta magica (riferimento locale della parete)
    GLboolean lt = glIsEnabled(GL_LIGHTING); glDisable(GL_LIGHTING);
    col(.6f, .8f, 1.f); rect(-.9f, 0, .9f, 2.7f, .16f);
    for (int k = 0; k < 4; k++) {
        float x = -.7f + k * .46f, j = .05f * sinf(g_time * 60 + k);
        strip({{x, 2.7f, .17f}, {x + .08f + j, 2.2f, .17f}, {x - .06f, 1.7f, .17f}, {x + .10f - j, 1.1f, .17f}, {x, .5f, .17f}, {x + .05f, 0, .17f}});
    }
    if (lt) glEnable(GL_LIGHTING);
}
