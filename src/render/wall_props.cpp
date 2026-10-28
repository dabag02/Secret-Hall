// Livello render — Oggetti appesi alle pareti: camino, spada, scudo (con i loro ganci) e torce a muro.
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void drawFireplace() {                 // cx=0 nel riferimento locale
    col(.40f, .46f, .55f);
    boxAt(-.95f, .8f, .3f, .3f, 1.6f, .6f);  // montanti
    boxAt(.95f, .8f, .3f, .3f, 1.6f, .6f);
    boxAt(0, 1.7f, .35f, 2.5f, .2f, .7f);    // mensola
    boxAt(0, 2.9f, .25f, 2.1f, 2.2f, .5f);   // cappa
    boxAt(0, .05f, .6f, 2.6f, .1f, 1.0f);    // focolare
    col(.28f, .32f, .40f);
    rect(-.8f, 0, .8f, 1.6f, .0f);           // fondo
    line(-.8f, 1.6f, 0, -.8f, 1.6f, .45f); line(.8f, 1.6f, 0, .8f, 1.6f, .45f);
    for (int k = 1; k < 4; k++) line(-.8f + .4f * k, 0, 0, -.8f + .4f * k, 1.6f, 0);
    if (g_fireLit) col(.95f, .45f, .15f); else col(.45f, .38f, .35f);   // tronchi (incandescenti se accesi) e cenere
    coneAlongX(-.15f, .12f, .25f, .08f, .07f, .7f);
    coneAlongX(.15f, .12f, .45f, .07f, .07f, .65f);
    coneAlongX(0.f, .26f, .35f, .07f, .06f, .6f);
    col(.55f, .55f, .58f);
    arc({0, .11f, .35f}, {.55f, 0, 0}, {0, 0, .3f}, 0, 2 * PI, 16);
    if (g_fireLit) {
        col(1.f, .50f, .15f); ringXZ({0, .14f, .35f}, .45f, .25f, 14);          // brace
        for (int k = 0; k < 3; k++) { PM f; glTranslatef((k - 1) * .30f, .30f, .35f); glScalef(2.6f, 2.6f, 2.6f); drawFlame(k * 1.7f); }
    }
    col(.62f, .67f, .78f);                   // alari
    for (int s = -1; s <= 1; s += 2) { boxAt(s * .6f, .2f, .35f, .05f, .4f, .05f); boxAt(s * .6f, .02f, .35f, .05f, .04f, .5f); }
}

void drawSword() {                     // appesa sulla cappa, z locale ~.53
    const float z = .54f, y = 2.7f;
    col(.82f, .88f, .98f);                   // lama
    strip({{-.7f, y + .035f, z}, {.80f, y + .03f, z}, {.98f, y, z}, {.80f, y - .03f, z}, {-.7f, y - .035f, z}});
    line(-.7f, y, z, .96f, y, z);
    col(.96f, .72f, .22f);                   // elsa
    line(-.7f, y + .16f, z, -.7f, y - .16f, z);
    rect(-.73f, y - .16f, -.67f, y + .16f, z);
    line(-.9f, y + .02f, z, -.7f, y + .02f, z); line(-.9f, y - .02f, z, -.7f, y - .02f, z);
    for (int i = 1; i < 5; i++) { float x = -.7f - .04f * i; line(x, y - .02f, z, x, y + .02f, z); }
    { PM _; glTranslatef(-.96f, y, z); sphere(.04f, .04f, .04f, 2, 6); }
}

void drawSwordHooks() {                // restano sulla cappa anche quando la spada è stata presa
    const float z = .54f, y = 2.7f;
    col(.62f, .67f, .78f);
    for (float x : {-.45f, .45f}) { boxAt(x, y - .06f, z - .02f, .05f, .08f, .06f); boxAt(x, y + .06f, z - .02f, .05f, .08f, .06f); }
}

std::vector<V> shieldOutline(float s) {
    std::vector<V> p;
    auto P = [&](float x, float y) { p.push_back({x * s, 2.1f + (y - 2.1f) * s, .045f}); };
    P(-.42f, 2.6f); P(.42f, 2.6f); P(.42f, 2.1f);
    for (int i = 1; i <= 8; i++) {
        float t = i / 8.f, u = 1 - t;
        P(u * u * .42f + 2 * u * t * .40f, u * u * 2.1f + 2 * u * t * 1.6f + t * t * 1.35f);
    }
    for (int i = 7; i >= 0; i--) {
        float t = i / 8.f, u = 1 - t;
        P(-(u * u * .42f + 2 * u * t * .40f), u * u * 2.1f + 2 * u * t * 1.6f + t * t * 1.35f);
    }
    return p;
}

void drawShield() {
    col(.35f, .55f, .95f);
    strip(shieldOutline(1.0f), true);
    strip(shieldOutline(.82f), true);
    col(.96f, .78f, .30f);                   // emblema: croce e bordo centrale
    line(0, 2.52f, .05f, 0, 1.50f, .05f); line(-.33f, 2.25f, .05f, .33f, 2.25f, .05f);
    ringXY({0, 2.25f, .05f}, .08f, .08f, 12);
    line(-.33f, 2.25f, .05f, -.33f, 2.52f, .05f); line(.33f, 2.25f, .05f, .33f, 2.52f, .05f);
}

void drawShieldHooks() {
    col(.62f, .67f, .78f);
    line(-.2f, 2.6f, .045f, -.2f, 2.68f, .0f); line(.2f, 2.6f, .045f, .2f, 2.68f, .0f);
}

void drawWallTorches() {
    for (int i = 0; i < 4; i++) {
        PM _; wallFrame(W[i], TORCH_U[i]);
        col(.62f, .67f, .78f);                                   // staffa di ferro
        boxAt(0, TORCH_Y, .015f, .12f, .16f, .03f);
        line(0, TORCH_Y, .03f, 0, TORCH_Y, TORCH_Z);
        PM t; glTranslatef(0, TORCH_Y, TORCH_Z); glRotatef(TORCH_TILT, 1, 0, 0);
        ringXZ({0, 0, 0}, .038f, .038f, 8);
        if (g_wt[i].present) drawTorchBody(g_wt[i].lit);
    }
}
