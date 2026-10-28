// ============================================================================
//  LA SALA SEGRETA — dungeon 3D in wireframe, navigabile in prima persona
//  C++17 + SDL2 + OpenGL (pipeline fissa, nessuna libreria extra: niente GLU/GLEW)
//
//  Compilazione (Linux):
//    Arch:  sudo pacman -S --needed sdl2 sdl2_ttf mesa urw-base35-fonts
//    g++ -std=c++17 -O2 sala_segreta.cpp -o sala_segreta $(sdl2-config --cflags --libs) -lSDL2_ttf -lGL
//
//  Comandi:
//    W A S D / mouse ... muoversi e guardare      Frecce ........ guardare (alternativa al mouse)
//    Shift ............. correre                   E ............. interagire (mirino giallo)
//    Tab ............... cattura/rilascia mouse     Esc ........... esci (o chiude l'inventario)
//    I ................. inventario: clic = mano destra, clic destro / Maiusc+1-9 = mano sinistra
//    F ................. (tenuto premuto) avvicina le due torce che hai in mano
//    E tenuto premuto .. trasporta l'oggetto puntato; rilasciando E cade ai tuoi piedi
//    Inizio partita: minigioco dei dadi (SPAZIO per lanciare) che definisce le caratteristiche
//    Nell'inventario: trascina la figura col mouse (o frecce sx/dx) per ruotarla a 360 gradi
//  Torce: E per staccarle/riappenderle; una torcia accesa avvicinata a una spenta la accende.
//  Messaggi e suggerimenti compaiono in un riquadro in basso (testo bianco, serif tipo Book
//  Antiqua/Palatino, corpo 14, sfondo nero semitrasparente). Font: variabile FONT=/percorso.ttf
//  per forzarlo; altrimenti cerca Book Antiqua/Palatino/P052/TeX Gyre Pagella, poi DejaVu Serif.
//
//  Opzione di test:  ./sala_segreta --shot out.ppm [x z yaw_gradi pitch_gradi] [--demo] [--inv] [--fig gradi] [--skipdice] [--dicetime s] [--carry] [--menu]
// ============================================================================
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <unistd.h>
#include <utility>
#include <string>
#include <vector>

static const float PI = 3.14159265358979f;

// Stato di gioco condiviso (definito presto perché usato dai disegni)
static bool g_tintHot = false;                 // oggetti roventi: colore pulsante rosso-arancio
static float g_hotPulse = 0;
static int g_rackState = 0;                    // rastrelliera: 0 intatta, 1 in fiamme, 2 bruciata
static float g_rackT = 0;
static bool g_skelFree = false;                // le braccia dello scheletro non sono più sostenute dal tavolo
static float g_zapT = 0, g_zapCool = 0; static int g_zapDoor = 0;     // scarica della porta magica
static float g_poison = 0;                     // secondi di intossicazione rimasti
static int g_hp = 1;                           // punti vita correnti
static bool g_dead = false; static std::string g_deathCause;
static bool g_fireLit = false;                 // fuoco nel camino
static int g_tableState = 0;                   // 0 intatto, 1 in fiamme, 2 bruciato
static float g_tableT = 0;
static bool g_rackHas[9] = {true, true, true, true, true, true, true, true, true};   // corazza, spalline s/d, gambali s/d, bracciali s/d, stivali s/d
static bool g_equip[10] = {};                  // indossati: elmo, corazza, spallina s/d, bracciale s/d, gambale s/d, stivale s/d
static bool g_chestGem = true, g_chestCoins = true;
static void drawFlame(float phase = 0);

// ----------------------------------------------------------------------------
//  Primitive wireframe
// ----------------------------------------------------------------------------
struct V { float x, y, z; };
struct PM { PM() { glPushMatrix(); } ~PM() { glPopMatrix(); } };

static inline void col(float r, float g, float b) { if (g_tintHot) glColor3f(1.f, .28f + .18f * g_hotPulse, .06f); else glColor3f(r, g, b); }

// le linee lunghe vengono spezzate (<= 0,5 m) perché l'illuminazione è calcolata per vertice
static void line(V a, V b) {
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    int n = std::max(1, (int)ceilf(sqrtf(dx * dx + dy * dy + dz * dz) / .5f));
    glBegin(GL_LINES);
    for (int i = 0; i < n; i++) {
        float t0 = (float)i / n, t1 = (float)(i + 1) / n;
        glVertex3f(a.x + dx * t0, a.y + dy * t0, a.z + dz * t0);
        glVertex3f(a.x + dx * t1, a.y + dy * t1, a.z + dz * t1);
    }
    glEnd();
}
static void line(float x1, float y1, float z1, float x2, float y2, float z2) {
    line(V{x1, y1, z1}, V{x2, y2, z2});
}
static void strip(const std::vector<V>& p, bool closed = false) {
    glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (auto& q : p) glVertex3f(q.x, q.y, q.z);
    glEnd();
}
// arco di ellisse: c + a*cos(t) + b*sin(t)
static void arc(V c, V a, V b, float t0, float t1, int n) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= n; i++) {
        float t = t0 + (t1 - t0) * i / n, cs = cosf(t), sn = sinf(t);
        glVertex3f(c.x + a.x * cs + b.x * sn, c.y + a.y * cs + b.y * sn, c.z + a.z * cs + b.z * sn);
    }
    glEnd();
}
static void ringXZ(V c, float rx, float rz, int n = 16) { arc(c, {rx, 0, 0}, {0, 0, rz}, 0, 2 * PI, n); }
static void ringXY(V c, float rx, float ry, int n = 16) { arc(c, {rx, 0, 0}, {0, ry, 0}, 0, 2 * PI, n); }
static void ringYZ(V c, float ry, float rz, int n = 16) { arc(c, {0, ry, 0}, {0, 0, rz}, 0, 2 * PI, n); }

static void rect(float x0, float y0, float x1, float y1, float z) {
    strip({{x0, y0, z}, {x1, y0, z}, {x1, y1, z}, {x0, y1, z}}, true);
}
// parallelepipedo centrato nell'origine
static void box(float w, float h, float d) {
    float x = w / 2, y = h / 2, z = d / 2;
    for (int sy = -1; sy <= 1; sy += 2) {
        line(-x, sy * y, -z, x, sy * y, -z); line(x, sy * y, -z, x, sy * y, z);
        line(x, sy * y, z, -x, sy * y, z);   line(-x, sy * y, z, -x, sy * y, -z);
    }
    line(-x, -y, -z, -x, y, -z); line(x, -y, -z, x, y, -z);
    line(x, -y, z, x, y, z);     line(-x, -y, z, -x, y, z);
}
static void boxAt(float cx, float cy, float cz, float w, float h, float d) {
    PM _; glTranslatef(cx, cy, cz); box(w, h, d);
}
// tronco di cono lungo +Y, base nell'origine
static void cone(float r0, float r1, float h, int seg = 12, int vert = -1) {
    if (vert < 0) vert = seg;
    arc({0, 0, 0}, {r0, 0, 0}, {0, 0, r0}, 0, 2 * PI, seg);
    arc({0, h, 0}, {r1, 0, 0}, {0, 0, r1}, 0, 2 * PI, seg);
    glBegin(GL_LINES);
    for (int i = 0; i < vert; i++) {
        float t = 2 * PI * i / vert, c = cosf(t), s = sinf(t);
        glVertex3f(r0 * c, 0, r0 * s); glVertex3f(r1 * c, h, r1 * s);
    }
    glEnd();
}
// cupola/sfera ellissoidale tra le latitudini p0..p1 (radianti)
static void dome(float rx, float ry, float rz, float p0, float p1, int nLat, int nLon) {
    for (int i = 0; i <= nLat; i++) {
        float p = p0 + (p1 - p0) * i / nLat, c = cosf(p);
        if (c < 0.02f) continue;
        arc({0, ry * sinf(p), 0}, {rx * c, 0, 0}, {0, 0, rz * c}, 0, 2 * PI, nLon * 2);
    }
    for (int j = 0; j < nLon; j++) {
        float t = 2 * PI * j / nLon, ct = cosf(t), st = sinf(t);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= nLat * 2; i++) {
            float p = p0 + (p1 - p0) * i / (nLat * 2), c = cosf(p);
            glVertex3f(rx * c * ct, ry * sinf(p), rz * c * st);
        }
        glEnd();
    }
}
static void sphere(float rx, float ry, float rz, int nLat = 4, int nLon = 8) {
    dome(rx, ry, rz, -PI / 2, PI / 2, nLat, nLon);
}
// cilindro/cono disteso lungo l'asse X, centrato in (cx,cy,cz)
static void coneAlongX(float cx, float cy, float cz, float r0, float r1, float len, int seg = 8) {
    PM _; glTranslatef(cx, cy, cz); glRotatef(-90, 0, 0, 1); glTranslatef(0, -len / 2, 0);
    cone(r0, r1, len, seg);
}

// ----------------------------------------------------------------------------
//  Geometria della stanza
//  Sistema: Y verso l'alto, stanza 10x10 m, altezza 4 m, x,z in [-5,5].
//  Ogni parete ha un riferimento locale: x = lungo il muro, y = alto, z = verso l'interno.
// ----------------------------------------------------------------------------
static const float WL = 10.0f, WH = 4.0f;
struct Hole { float u0, u1, v1; };
struct Wall { float ox, oz, ux, uz, nx, nz; std::vector<Hole> holes; };

static Wall W[4] = {
    {-5, -5,  1, 0,  0,  1, {{4.2f, 5.8f, 2.6f}}},   // Nord (z=-5): porta in x=0
    { 5,  5, -1, 0,  0, -1, {{4.2f, 5.8f, 2.6f}}},   // Sud  (z=+5): porta in x=0
    { 5, -5,  0, 1, -1,  0, {{7.6f, 9.2f, 2.6f}}},   // Est  (x=+5): porta in z=+3.4, camino in z=0
    {-5, -5,  0, 1,  1,  0, {{0.8f, 2.4f, 2.6f}}},   // Ovest(x=-5): porta in z=-3.4, scudo in z=0
};
static const float DOOR_U[4] = {5.0f, 5.0f, 8.4f, 1.6f};

static void wallFrame(const Wall& w, float u) {
    float m[16] = {w.ux, 0, w.uz, 0,  0, 1, 0, 0,  w.nx, 0, w.nz, 0,
                   w.ox + w.ux * u, 0, w.oz + w.uz * u, 1};
    glMultMatrixf(m);
}

static void wallStones(const Wall& w) {
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

static void drawFloor() {
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

static void drawCeiling() {
    col(.55f, .33f, .14f);                    // travi di quercia
    for (int i = 0; i < 4; i++) {
        float x = -3.75f + i * 2.5f;
        boxAt(x, WH - .15f, 0, .3f, .3f, 10.f);
    }
}

// ----------------------------------------------------------------------------
//  Porte e quadri (nel riferimento locale della parete)
// ----------------------------------------------------------------------------
static void drawDoor() {
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

static void drawPainting(int variant) {
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

// ----------------------------------------------------------------------------
//  Camino, spada, scudo (parete Est / Ovest)
// ----------------------------------------------------------------------------
static void drawFireplace() {                 // cx=0 nel riferimento locale
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

static void drawSword() {                     // appesa sulla cappa, z locale ~.53
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
static void drawSwordHooks() {                // restano sulla cappa anche quando la spada è stata presa
    const float z = .54f, y = 2.7f;
    col(.62f, .67f, .78f);
    for (float x : {-.45f, .45f}) { boxAt(x, y - .06f, z - .02f, .05f, .08f, .06f); boxAt(x, y + .06f, z - .02f, .05f, .08f, .06f); }
}

static std::vector<V> shieldOutline(float s) {
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
static void drawShield() {
    col(.35f, .55f, .95f);
    strip(shieldOutline(1.0f), true);
    strip(shieldOutline(.82f), true);
    col(.96f, .78f, .30f);                   // emblema: croce e bordo centrale
    line(0, 2.52f, .05f, 0, 1.50f, .05f); line(-.33f, 2.25f, .05f, .33f, 2.25f, .05f);
    ringXY({0, 2.25f, .05f}, .08f, .08f, 12);
    line(-.33f, 2.25f, .05f, -.33f, 2.52f, .05f); line(.33f, 2.25f, .05f, .33f, 2.52f, .05f);
}
static void drawShieldHooks() {
    col(.62f, .67f, .78f);
    line(-.2f, 2.6f, .045f, -.2f, 2.68f, .0f); line(.2f, 2.6f, .045f, .2f, 2.68f, .0f);
}

// ----------------------------------------------------------------------------
//  Elmo e rastrelliera con l'armatura
// ----------------------------------------------------------------------------
static void drawHelm(float r) {               // fronte verso +z
    dome(r, r * 1.1f, r, 0, PI / 2, 3, 8);
    { PM _; glTranslatef(0, -.75f * r, 0); cone(r * .95f, r, .75f * r, 12, 12); }
    auto zf = [&](float x) { return sqrtf(std::max(0.f, r * r - x * x)) * 1.03f; };
    strip({{-.55f * r, -.20f * r, zf(.55f * r)}, {.55f * r, -.20f * r, zf(.55f * r)},
           {.55f * r, -.05f * r, zf(.55f * r)}, {-.55f * r, -.05f * r, zf(.55f * r)}}, true);   // feritoia
    line(0, .35f * r, r * 1.03f, 0, -.70f * r, r * 1.03f);                                       // nasale
    for (int i = 0; i < 4; i++) { float y = -.35f * r - .1f * r * i; line(.2f * r, y, zf(.2f * r), .5f * r, y, zf(.5f * r)); }
}

static void drawCuirassModel() {              // origine alla base (appesa alla barra)
    col(.70f, .78f, .92f);
    PM _; glScalef(1, 1, .7f);
    cone(.17f, .22f, .5f, 12, 12);
    ringXZ({0, .5f, 0}, .08f, .08f, 10);
    line(0, .02f, .22f, 0, .5f, .17f);
    ringYZ({.20f, .36f, 0}, .06f, .06f, 8); ringYZ({-.20f, .36f, 0}, .06f, .06f, 8);
}
static void drawPauldronModel() { col(.70f, .78f, .92f); dome(.14f, .10f, .14f, 0, PI / 2, 3, 8); }
static void drawGreaveModel() { col(.70f, .78f, .92f); cone(.07f, .055f, .45f, 8); }       // lungo +Y
static void drawVambraceModel() { col(.70f, .78f, .92f); cone(.05f, .04f, .30f, 8); }
static void drawBootModel() { col(.70f, .78f, .92f); cone(.065f, .055f, .30f, 8, 8); boxAt(0, .04f, .10f, .10f, .08f, .26f); }
static void drawGemModel() {
    col(.4f, 1.f, 1.f);
    V t{0, .07f, 0}, b{0, -.07f, 0}, p[4] = {{.05f, 0, 0}, {0, 0, .05f}, {-.05f, 0, 0}, {0, 0, -.05f}};
    for (int i = 0; i < 4; i++) { line(t, p[i]); line(b, p[i]); line(p[i], p[(i + 1) % 4]); }
}
static void drawBookModel() {                  // libro aperto appoggiato, origine al centro della base
    col(.55f, .18f, .14f); boxAt(0, .008f, 0, .40f, .016f, .28f);
    col(.95f, .92f, .80f);
    { PM _; glTranslatef(-.095f, .026f, 0); glRotatef(4, 0, 0, 1); box(.18f, .02f, .25f); }
    { PM _; glTranslatef(.095f, .026f, 0); glRotatef(-4, 0, 0, 1); box(.18f, .02f, .25f); }
    col(.45f, .40f, .30f);
    for (int k = 0; k < 6; k++) { float z = -.09f + k * .036f; line(-.17f, .038f, z, -.03f, .038f, z); line(.03f, .038f, z, .17f, .038f, z); }
    line(0, .02f, -.125f, 0, .02f, .125f);
}
static void drawAshModel() {                   // cenere tossica (verdastra)
    col(.50f, .62f, .40f);
    ringXZ({0, .01f, 0}, .16f, .12f, 12); ringXZ({0, .03f, 0}, .10f, .08f, 10); ringXZ({0, .05f, 0}, .04f, .03f, 8);
    col(.65f, .75f, .50f);
    for (int k = 0; k < 6; k++) { float t = k * 1.047f; line(.16f * cosf(t), .01f, .12f * sinf(t), .04f * cosf(t + .4f), .05f, .03f * sinf(t + .4f)); }
}
static void drawCoinsModel() {                 // due piccole pile di monete
    col(1.f, .85f, .30f);
    for (int k = 0; k < 3; k++) { PM _; glTranslatef(0, .012f + .022f * k, 0); cone(.06f, .06f, .018f, 12, 6); }
    for (int k = 0; k < 2; k++) { PM _; glTranslatef(.12f, .012f + .022f * k, .06f); cone(.06f, .06f, .018f, 12, 6); }
}

static void drawRack() {                      // locale: x lungo il muro, z verso la stanza
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
        if (g_rackHas[1 + a]) { PM _; glTranslatef(sd * .58f, 1.25f, .25f); drawPauldronModel(); }
        if (g_rackHas[3 + a]) { PM _; glTranslatef(sd * .70f, .90f, .25f); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.225f, 0); drawGreaveModel(); }
        if (g_rackHas[5 + a]) { PM _; glTranslatef(sd * .22f, .88f, .25f); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.15f, 0); drawVambraceModel(); }
        if (g_rackHas[7 + a]) { PM _; glTranslatef(sd * .65f, .32f, .18f); drawBootModel(); }
    }
    if (g_rackHas[0]) { PM _; glTranslatef(0, .95f, .25f); drawCuirassModel(); }
}

static void drawRackFire() {                   // nel riferimento locale della parete ovest
    for (int i = 0; i < 5; i++) { PM _; glTranslatef(-1.0f + i * .5f, .20f + (i & 1) * .75f, .25f); glScalef(2.2f, 2.2f, 2.2f); drawFlame(i * 1.1f); }
    for (int sd = -1; sd <= 1; sd += 2) { PM _; glTranslatef(sd * 1.15f, .95f, .25f); glScalef(2.6f, 2.6f, 2.6f); drawFlame(sd * 2.3f); }
}
static void drawRackRemains() {                // rastrelliera carbonizzata: monconi, assi cadute, cenere
    col(.30f, .26f, .23f);
    for (int sd = -1; sd <= 1; sd += 2) { PM _; glTranslatef(sd * 1.15f, 0, .25f); glRotatef(-sd * 6.f, 0, 0, 1); cone(.045f, .03f, .42f, 8); }
    { PM _; glTranslatef(-.2f, .04f, .5f); glRotatef(15, 0, 1, 0); box(2.2f, .06f, .07f); }
    { PM _; glTranslatef(.3f, .035f, .8f); glRotatef(-30, 0, 1, 0); box(1.2f, .05f, .45f); }
    { PM _; glTranslatef(0, .05f, .35f); glRotatef(80, 0, 1, 0); box(1.0f, .05f, .06f); }
    col(.42f, .40f, .38f); ringXZ({0, .01f, .4f}, .9f, .5f, 16); ringXZ({0, .03f, .4f}, .6f, .3f, 14);
    col(1.f, .40f, .12f); ringXZ({.2f, .06f, .5f}, .08f, .06f, 8);
}

// ----------------------------------------------------------------------------
//  Tavolo, sedia, scheletro, scrigno, secchio, chiave
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//  Oggetti, inventario e torce
// ----------------------------------------------------------------------------
struct Item { int type = 0; bool lit = false; };      // type: 0 vuoto, 1 chiave, 2 torcia
static Item g_inv[9], g_hand[2];                        // g_hand[0] = mano sinistra, g_hand[1] = destra
struct WallTorch { bool present, lit; };
static WallTorch g_wt[4] = {{true, true}, {true, true}, {true, true}, {true, false}};   // N, S, E accese; O spenta
static const float TORCH_U[4] = {2.2f, 2.2f, 1.6f, 8.4f};      // posizione lungo la parete i
static const float TORCH_Y = 1.9f, TORCH_Z = .10f, TORCH_TILT = 25.f;
static bool g_keyInBucket = true, g_chestOpen = false, g_swordOnWall = true, g_shieldOnWall = true;
static float g_figAngle = .5f;

// Oggetti trasportabili "sciolti" nella stanza (secchio, scrigno, elmo, e ciò che viene lasciato cadere)
enum { LT_BUCKET = 1, LT_CHEST, LT_ARMOR, LT_SWORD, LT_SHIELD, LT_TORCH, LT_GEM, LT_COINS, LT_BOOK, LT_KEY, LT_ASH };
struct Loose { int type; bool lit; V pos; float yaw, vy; int sub = 0; float hot = 0, burn = 0; };   // sub: pezzo d'armatura; hot: s di calore; burn: s di combustione
static std::vector<Loose> g_loose = {
    {LT_BUCKET, false, {1.6f, 0.f, -4.4f}, 0.f, 0.f},
    {LT_CHEST,  false, {-.2f, .9f, 0.f},   0.f, 0.f},
    {LT_ARMOR,  false, {-3.95f, 0.f, .9f}, .6f, 0.f, 10},          // l'elmo a terra
    {LT_BOOK,   false, {1.05f, .9f, 0.f},  .25f, 0.f},             // libro aperto sul tavolo, di fronte allo scheletro
};
static bool g_carrying = false;
static Loose g_carry{};
static float floorY(float x, float z) {                          // piano del tavolo, focolare o pavimento
    if (x > 3.9f && x < 4.85f && fabsf(z) < .9f) return .1f;
    return (g_tableState != 2 && fabsf(x) < 1.5f && fabsf(z) < .65f) ? .9f : 0.f;
}
static V looseFlamePos(const Loose& o) {                        // fiamma di una torcia a terra (testa verso +x locale)
    float c = cosf(o.yaw), sn = sinf(o.yaw);
    return {o.pos.x + .44f * c, o.pos.y + .18f, o.pos.z - .44f * sn};
}
static const char* armorArt(int t) {
    switch (t) {
        case 10: return "l'elmo"; case 11: return "la corazza"; case 12: return "una spallina";
        case 13: return "un gambale"; case 14: return "un bracciale"; case 15: return "uno stivale";
    }
    return "il pezzo";
}
static const char* looseName(const Loose& o) {
    switch (o.type) {
        case LT_BUCKET: return "il secchio"; case LT_CHEST: return "lo scrigno"; case LT_ARMOR: return armorArt(o.sub);
        case LT_SWORD: return "la spada"; case LT_SHIELD: return "lo scudo"; case LT_TORCH: return "la torcia";
        case LT_GEM: return "la gemma"; case LT_COINS: return "le monete d'oro";
        case LT_BOOK: return "il libro"; case LT_KEY: return "la chiave"; case LT_ASH: return "le ceneri tossiche";
    }
    return "l'oggetto";
}
static Item itemOfLoose(const Loose& o) {
    switch (o.type) {
        case LT_SWORD: return Item{3, false}; case LT_SHIELD: return Item{4, false}; case LT_TORCH: return Item{2, o.lit};
        case LT_ARMOR: return Item{o.sub, false}; case LT_GEM: return Item{6, false}; case LT_COINS: return Item{7, false};
        case LT_BOOK: return Item{8, false}; case LT_KEY: return Item{1, false}; case LT_BUCKET: return Item{5, false};
    }
    return Item{};
}
static Loose looseOfItem(const Item& it) {      // l'oggetto lasciato cadere dalle mani
    Loose o{}; o.lit = it.lit;
    switch (it.type) {
        case 1: o.type = LT_KEY; break; case 2: o.type = LT_TORCH; break; case 3: o.type = LT_SWORD; break; case 4: o.type = LT_SHIELD; break;
        case 5: o.type = LT_BUCKET; break; case 6: o.type = LT_GEM; break; case 7: o.type = LT_COINS; break; case 8: o.type = LT_BOOK; break;
        default: o.type = LT_ARMOR; o.sub = it.type;
    }
    return o;
}
static float g_lid = 0, g_time = 0;

static bool hasKey() {
    for (auto& h : g_hand) if (h.type == 1) return true;
    for (auto& s : g_inv) if (s.type == 1) return true;
    return false;
}
static int handWith(int type) { if (g_hand[1].type == type) return 1; if (g_hand[0].type == type) return 0; return -1; }
// mette l'oggetto in mano (prima la destra) e/o nell'inventario; false se non c'è posto
static float totalKg(); static float itemKg(int type);
static std::string g_giveFail;
static std::string fmtKg(float k) { char b[32]; snprintf(b, sizeof b, "%.1f", k); std::string r = b; for (auto& c : r) if (c == '.') c = ','; return r; }
static const float MAX_KG = 20.f;                                  // peso massimo trasportabile
static std::string weightFail(float kg) {                          // "" se si può portare, altrimenti il motivo
    if (totalKg() + kg <= MAX_KG + 1e-3f) return "";
    return "Troppo pesante: porti già " + fmtKg(totalKg()) + " kg su " + fmtKg(MAX_KG) + " e questo ne pesa " + fmtKg(kg) + ".";
}
static std::string giveFailMsg() { return g_giveFail.empty() ? "Hai mani e inventario pieni (tasto I)." : g_giveFail; }
static bool giveItem(Item it, bool preferHand) {
    g_giveFail = weightFail(itemKg(it.type));
    if (!g_giveFail.empty()) return false;
    auto toHand = [&]() { if (!g_hand[1].type) { g_hand[1] = it; return true; } if (!g_hand[0].type) { g_hand[0] = it; return true; } return false; };
    auto toInv = [&]() { for (auto& s : g_inv) if (!s.type) { s = it; return true; } return false; };
    bool ok = preferHand ? (toHand() || toInv()) : (toInv() || toHand());
    if (!ok) g_giveFail = "Hai mani e inventario pieni (tasto I).";
    return ok;
}
static std::string itemName(const Item& it) {
    switch (it.type) {
        case 1: return "Chiave di bronzo";
        case 2: return it.lit ? "Torcia accesa" : "Torcia spenta";
        case 3: return "Spada lunga";
        case 4: return "Scudo con croce dorata";
        case 5: return g_keyInBucket ? "Secchio di cenere (da frugare)" : "Secchio di legno";
        case 6: return "Gemma";
        case 7: return "Monete d'oro";
        case 8: return "Libro";
        case 10: return "Elmo"; case 11: return "Corazza"; case 12: return "Spallina";
        case 13: return "Gambale"; case 14: return "Bracciale"; case 15: return "Stivale";
    }
    return "";
}
static bool isArmor(int t) { return t >= 10 && t <= 15; }
static const int SLOT_TYPE[10] = {10, 11, 12, 12, 14, 14, 13, 13, 15, 15};
static const char* SLOT_NAME[10] = {"Elmo", "Corazza", "Spallina sinistra", "Spallina destra", "Bracciale sinistro", "Bracciale destro",
                                    "Gambale sinistro", "Gambale destro", "Stivale sinistro", "Stivale destro"};
static const int RACK_TYPE[9] = {11, 12, 12, 13, 13, 14, 14, 15, 15};
static const V RACK_POS[9] = {{-4.75f, 1.2f, 0}, {-4.75f, 1.25f, -.58f}, {-4.75f, 1.25f, .58f}, {-4.75f, .9f, -.70f}, {-4.75f, .9f, .70f},
                              {-4.75f, .88f, -.22f}, {-4.75f, .88f, .22f}, {-4.82f, .35f, -.65f}, {-4.82f, .35f, .65f}};
// ---- pesi (kg) ----
static float itemKg(int t) {
    switch (t) {
        case 1: return .15f; case 2: return 1.2f; case 3: return 1.6f; case 4: return 3.0f; case 5: return 3.0f;
        case 6: return .1f; case 7: return .9f; case 8: return 1.2f;
        case 10: return 1.8f; case 11: return 5.0f; case 12: return 1.0f; case 13: return 1.5f; case 14: return .8f; case 15: return 1.2f;
    }
    return 0.f;
}
static float looseKg(const Loose& o) { return o.type == LT_CHEST ? 12.f : o.type == LT_ASH ? 0.f : itemKg(itemOfLoose(o).type); }
static float invKg() { float w = 0; for (auto& s : g_inv) w += itemKg(s.type); return w; }
static float handKg() { return itemKg(g_hand[0].type) + itemKg(g_hand[1].type); }
static float wornKg() { float w = 0; for (int i = 0; i < 10; i++) if (g_equip[i]) w += itemKg(SLOT_TYPE[i]); return w; }
static float totalKg() { return invKg() + handKg() + wornKg() + (g_carrying ? looseKg(g_carry) : 0.f); }
static V wallPoint(const Wall& w, float u, float lx, float ly, float lz) {
    return {w.ox + w.ux * (u + lx) + w.nx * lz, ly, w.oz + w.uz * (u + lx) + w.nz * lz};
}
// punto lungo l'asse della torcia appesa i (t = distanza dal punto di appoggio)
static V torchAxisPos(int i, float t) {
    float a = TORCH_TILT * PI / 180.f;
    return wallPoint(W[i], TORCH_U[i], 0, TORCH_Y + cosf(a) * t, TORCH_Z + sinf(a) * t);
}

static void drawFlame(float phase) {          // origine = base della fiamma, asse +Y; non subisce l'illuminazione
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

static void drawTorchBody(bool lit, bool withFlame = true) {         // origine = punto di appoggio, asse +Y (manico -0,30 .. testa +0,40)
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

static void drawWallTorches() {
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

static void drawKey() {                       // lunghezza ~0.42 unità, nel piano XY
    ringXY({0, 0, 0}, .06f, .06f, 14); ringXY({0, 0, 0}, .03f, .03f, 10);
    line(.06f, 0, 0, .42f, 0, 0);
    strip({{.30f, 0, 0}, {.30f, -.09f, 0}, {.36f, -.09f, 0}, {.36f, 0, 0}});
    strip({{.42f, 0, 0}, {.42f, -.06f, 0}, {.40f, -.06f, 0}});
}

static void drawTable() {
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

static void drawChair() {
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

static void bone(V a, V b, float r = .018f) {
    line(a, b); ringXZ(a, r, r, 6); ringXZ(b, r, r, 6);
}

// ---- scheletro: proprietà fisiche ----
// Peso: ossa ~9 kg (10,8 con l'elmo). Le braccia sono un doppio pendolo (omero e avambraccio+mano) con le masse qui sotto:
// appoggiate al tavolo finché esiste; quando il tavolo brucia e crolla, cadono ai fianchi per gravità, con smorzamento.
static const float SKELETON_KG = 9.0f, SKELETON_HELM_KG = 1.8f;
static const float ARM_L1 = .30f, ARM_L2 = .25f, ARM_M1 = .6f, ARM_M2 = .5f;     // lunghezze (m) e masse (kg)
struct ArmState { float t1, t2, w1, w2; };                                        // angoli dalla verticale (rad), + = verso il tavolo
static ArmState g_arm[2];
static void armPose(int sd, V& sh, V& el, V& wr) {
    const float a = 12.f * PI / 180.f;
    sh = {2.05f - .5f * sinf(a), .60f + .5f * cosf(a), sd * .19f};
    if (!g_skelFree) {                                    // mano appoggiata sul bordo del tavolo: cinematica inversa a due segmenti
        float dx = 1.46f - sh.x, dy = .93f - sh.y, D = std::min(sqrtf(dx * dx + dy * dy), ARM_L1 + ARM_L2 - 1e-3f);
        float beta = atan2f(dy, dx), ca = (ARM_L1 * ARM_L1 + D * D - ARM_L2 * ARM_L2) / (2 * ARM_L1 * D);
        float A = acosf(std::max(-1.f, std::min(1.f, ca)));
        float e1 = beta - A, e2 = beta + A, y1 = sh.y + ARM_L1 * sinf(e1), y2 = sh.y + ARM_L1 * sinf(e2), e = y1 < y2 ? e1 : e2;
        el = {sh.x + ARM_L1 * cosf(e), sh.y + ARM_L1 * sinf(e), sd * .23f};
        wr = {sh.x + D * cosf(beta), sh.y + D * sinf(beta), sd * .20f};
    } else {
        const ArmState& s = g_arm[sd < 0 ? 0 : 1];
        el = {sh.x - ARM_L1 * sinf(s.t1), sh.y - ARM_L1 * cosf(s.t1), sd * .23f};
        wr = {el.x - ARM_L2 * sinf(s.t2), el.y - ARM_L2 * cosf(s.t2), sd * .22f};
    }
}
static void updateSkeleton(float dt) {
    if (g_tableState == 2 && !g_skelFree) {               // viene a mancare l'appoggio: le braccia si staccano
        for (int h = 0; h < 2; h++) {
            V sh, el, wr; armPose(h ? 1 : -1, sh, el, wr);   // posa corrente (ancora a riposo) = condizione iniziale
            g_arm[h] = {atan2f(-(el.x - sh.x), -(el.y - sh.y)), atan2f(-(wr.x - el.x), -(wr.y - el.y)), 0.f, 0.f};
        }
        g_skelFree = true;
    }
    if (!g_skelFree) return;
    const float g = 9.81f, damp = .9f; const int N = 8; float h = dt / N;
    for (auto& a : g_arm)
        for (int i = 0; i < N; i++) {                     // doppio pendolo (equazioni di Lagrange) con attrito d'aria/giunto
            float d = a.t1 - a.t2, den = 2 * ARM_M1 + ARM_M2 - ARM_M2 * cosf(2 * d);
            float a1 = (-g * (2 * ARM_M1 + ARM_M2) * sinf(a.t1) - ARM_M2 * g * sinf(a.t1 - 2 * a.t2) -
                        2 * sinf(d) * ARM_M2 * (a.w2 * a.w2 * ARM_L2 + a.w1 * a.w1 * ARM_L1 * cosf(d))) / (ARM_L1 * den);
            float a2 = (2 * sinf(d) * (a.w1 * a.w1 * ARM_L1 * (ARM_M1 + ARM_M2) + g * (ARM_M1 + ARM_M2) * cosf(a.t1) +
                        a.w2 * a.w2 * ARM_L2 * ARM_M2 * cosf(d))) / (ARM_L2 * den);
            a.w1 += (a1 - damp * a.w1) * h; a.w2 += (a2 - damp * a.w2) * h; a.t1 += a.w1 * h; a.t2 += a.w2 * h;
            if (a.t1 < -.35f) { a.t1 = -.35f; if (a.w1 < 0) a.w1 *= -.2f; }          // la spalla non va oltre il busto
            if (a.t2 - a.t1 < -.05f) { a.t2 = a.t1 - .05f; if (a.w2 < a.w1) a.w2 = a.w1; }   // il gomito non si iperestende
        }
}

static void drawSkeleton() {
    col(.97f, .96f, .86f);
    {   // tutto ciò che sta nel riferimento "inclinato" del busto
        PM _; glTranslatef(2.05f, .60f, 0); glRotatef(12, 0, 0, 1);
        sphere(.10f, .06f, .17f, 2, 8);                     // bacino
        ringYZ({0, -.02f, .09f}, .03f, .03f, 6); ringYZ({0, -.02f, -.09f}, .03f, .03f, 6);
        for (int i = 0; i < 10; i++) ringXZ({0, .05f + i * .045f, 0}, .022f, .028f, 6);   // vertebre
        const float rz[7] = {.10f, .125f, .14f, .145f, .14f, .125f, .09f};
        std::vector<V> sternum;
        for (int i = 0; i < 7; i++) {                       // gabbia toracica
            float y = .19f + i * .052f;
            ringXZ({0, y, 0}, rz[i] * .68f, rz[i], 14);
            sternum.push_back({-rz[i] * .68f, y, 0});
        }
        strip(sternum);
        for (int i = 0; i < 6; i++) {
            float y0 = .19f + i * .052f, y1 = y0 + .052f;
            for (float t : {.9f, 1.5f, 2.3f, 2.9f, 3.4f, 3.9f, 4.7f, 5.4f}) {
                line(rz[i] * .68f * cosf(t), y0, rz[i] * sinf(t), rz[i + 1] * .68f * cosf(t), y1, rz[i + 1] * sinf(t));
            }
        }
        line(0, .53f, -.19f, 0, .53f, .19f);                // clavicole
        ringXZ({0, .52f, .19f}, .03f, .03f, 6); ringXZ({0, .52f, -.19f}, .03f, .03f, 6);
        {   // cranio
            PM s; glTranslatef(0, .66f, 0);
            sphere(.11f, .11f, .09f, 4, 8);
            ringYZ({-.100f, .005f, .04f}, .025f, .025f, 8);    // orbite
            ringYZ({-.100f, .005f, -.04f}, .025f, .025f, 8);
            strip({{-.105f, -.025f, 0}, {-.118f, -.050f, .012f}, {-.118f, -.050f, -.012f}}, true);   // naso
            { PM j; glTranslatef(-.02f, -.115f, 0); box(.10f, .04f, .10f); }                        // mandibola
            strip({{-.075f, -.08f, -.04f}, {-.075f, -.095f, -.02f}, {-.075f, -.08f, 0}, {-.075f, -.095f, .02f}, {-.075f, -.08f, .04f}});
            // elmo in testa
            PM h; glTranslatef(0, .045f, 0); glRotatef(-90, 0, 1, 0);
            col(.70f, .78f, .92f); drawHelm(.128f); col(.97f, .96f, .86f);
        }
    }
    for (int s = -1; s <= 1; s += 2) {                      // braccia: appoggiate al tavolo
        V sh, el, wr; armPose(s, sh, el, wr);
        bone(sh, el, .022f); bone(el, wr, .018f);
        { float ex = wr.x - el.x, ey = wr.y - el.y, L = sqrtf(ex * ex + ey * ey) + 1e-6f;
          for (int k = -1; k <= 2; k++) line(wr, V{wr.x + ex / L * .11f, wr.y + ey / L * .11f, wr.z + k * .022f}); }   // dita
        // gambe: bacino -> ginocchio -> caviglia -> piede
        V hip{2.00f, .56f, s * .09f}, kn{1.50f, .58f, s * .12f}, an{1.46f, .10f, s * .12f}, toe{1.24f, .04f, s * .12f};
        bone(hip, kn, .03f); bone(kn, an, .025f);
        line(an, toe); line(an, V{1.55f, .03f, s * .12f}); line(toe, V{1.24f, .04f, s * .12f + s * .0f});
    }
}

static void drawChest() {
    PM _;
    col(.80f, .45f, .16f);
    boxAt(0, .14f, 0, .8f, .28f, .5f);                      // corpo
    col(.62f, .67f, .78f);                                  // fasce metalliche
    for (float x : {-.28f, .28f}) {
        line(x, 0, .25f, x, .28f, .25f); line(x, .28f, -.25f, x, .28f, .25f); line(x, 0, -.25f, x, .28f, -.25f);
    }
    rect(-.05f, .10f, .05f, .24f, .252f);                   // piastra della serratura
    {   // coperchio a botte, incernierato sul bordo posteriore
        PM l; glTranslatef(0, .28f, -.25f); glRotatef(-g_lid, 1, 0, 0);
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
    if (!g_chestOpen) {                                     // lucchetto
        col(.96f, .72f, .22f);
        boxAt(0, .12f, .27f, .07f, .09f, .03f);
        arc({0, .165f, .27f}, {.025f, 0, 0}, {0, .04f, 0}, 0, PI, 8);
        ringXY({0, .12f, .286f}, .008f, .008f, 6);
    } else {                                                // contenuto: gemma e monete (se ancora dentro)
        if (g_chestGem) { PM g; glTranslatef(0, .22f, 0); glRotatef(g_time * 60, 0, 1, 0); drawGemModel(); }
        if (g_chestCoins) { PM c; glTranslatef(.1f, .02f, .05f); drawCoinsModel(); }
    }
}

static void drawBucketBody() {
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

// ----------------------------------------------------------------------------
//  La scena intera
// ----------------------------------------------------------------------------
static void drawArmorLying(int t) {            // pezzo d'armatura appoggiato a terra
    switch (t) {
        case 10: glTranslatef(0, .14f, 0); glRotatef(82, 0, 0, 1); glRotatef(35, 0, 1, 0); col(.70f, .78f, .92f); drawHelm(.13f); break;
        case 11: glTranslatef(0, .20f, 0); glRotatef(90, 0, 0, 1); glTranslatef(0, -.25f, 0); drawCuirassModel(); break;
        case 12: drawPauldronModel(); break;
        case 13: glTranslatef(0, .07f, 0); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.225f, 0); drawGreaveModel(); break;
        case 14: glTranslatef(0, .05f, 0); glRotatef(-90, 0, 0, 1); glTranslatef(0, -.15f, 0); drawVambraceModel(); break;
        case 15: glTranslatef(0, .065f, 0); glRotatef(-90, 0, 0, 1); drawBootModel(); break;
    }
}
static void drawLooseObj(const Loose& o) {
    PM _; glTranslatef(o.pos.x, o.pos.y, o.pos.z); glRotatef(o.yaw * 180.f / PI, 0, 1, 0);
    g_tintHot = o.hot > 0;                                          // pezzi roventi: colore pulsante
    switch (o.type) {
        case LT_BUCKET: drawBucketBody(); break;
        case LT_CHEST: drawChest(); break;
        case LT_ARMOR: drawArmorLying(o.sub); break;
        case LT_GEM: glTranslatef(0, .08f, 0); glRotatef(g_time * 40, 0, 1, 0); drawGemModel(); break;
        case LT_COINS: drawCoinsModel(); break;
        case LT_BOOK:
            drawBookModel();
            if (o.burn > 0) for (int k = 0; k < 3; k++) { PM f; glTranslatef(-.12f + k * .12f, .04f, (k - 1) * .04f); glScalef(1.5f, 1.5f, 1.5f); drawFlame(k * 1.3f); }
            break;
        case LT_KEY: glTranslatef(0, .02f, 0); glRotatef(-90, 1, 0, 0); glScalef(.7f, .7f, .7f); col(.80f, .55f, .15f); drawKey(); break;
        case LT_ASH: drawAshModel(); break;
        case LT_SWORD: glTranslatef(0, .03f, 0); glRotatef(-90, 1, 0, 0); glTranslatef(0, -2.7f, -.54f); drawSword(); break;
        case LT_SHIELD: glTranslatef(0, .03f, 0); glRotatef(-90, 1, 0, 0); glTranslatef(0, -2.0f, -.045f); drawShield(); break;
        case LT_TORCH: {
            glTranslatef(0, .04f, 0);
            { PM b; glRotatef(-90, 0, 0, 1); drawTorchBody(o.lit, false); }
            if (o.lit) { glTranslatef(.40f, 0, 0); drawFlame(); }
        } break;
    }
    g_tintHot = false;
}
static void drawZap() {                        // scarica della porta magica (riferimento locale della parete)
    GLboolean lt = glIsEnabled(GL_LIGHTING); glDisable(GL_LIGHTING);
    col(.6f, .8f, 1.f); rect(-.9f, 0, .9f, 2.7f, .16f);
    for (int k = 0; k < 4; k++) {
        float x = -.7f + k * .46f, j = .05f * sinf(g_time * 60 + k);
        strip({{x, 2.7f, .17f}, {x + .08f + j, 2.2f, .17f}, {x - .06f, 1.7f, .17f}, {x + .10f - j, 1.1f, .17f}, {x, .5f, .17f}, {x + .05f, 0, .17f}});
    }
    if (lt) glEnable(GL_LIGHTING);
}
static void drawLooseObjects() {
    for (auto& o : g_loose) drawLooseObj(o);
    if (g_carrying) drawLooseObj(g_carry);
}

static void drawTableFire() {
    for (int k = 0; k < 6; k++) {
        PM _; glTranslatef(-1.25f + k * .5f, .93f, (k & 1) ? .22f : -.22f); glScalef(2.2f, 2.2f, 2.2f); drawFlame(k * 1.3f);
    }
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) { PM _; glTranslatef(sx * 1.3f, .30f, sz * .45f); glScalef(1.6f, 1.6f, 1.6f); drawFlame((sx + 2 * sz) * 1.1f); }
}
static void drawTableRemains() {               // tavolo carbonizzato: monconi, assi cadute, cenere
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

static void drawWorld() {
    drawFloor();
    drawCeiling();
    for (int i = 0; i < 4; i++) wallStones(W[i]);
    for (int i = 0; i < 4; i++) {
        PM _; wallFrame(W[i], DOOR_U[i]); drawDoor(); drawPainting(i);
        if (g_zapT > 0 && g_zapDoor == i) drawZap();
    }
    { PM _; wallFrame(W[2], 5.f); drawFireplace(); drawSwordHooks(); if (g_swordOnWall) drawSword(); }
    { PM _; wallFrame(W[3], 5.f); drawShieldHooks(); if (g_shieldOnWall) drawShield();
      if (g_rackState == 2) drawRackRemains(); else drawRack();
      if (g_rackState == 1) drawRackFire(); }
    if (g_tableState == 2) drawTableRemains(); else drawTable();
    if (g_tableState == 1) drawTableFire();
    drawChair(); drawSkeleton(); drawWallTorches(); drawLooseObjects();
}

// ----------------------------------------------------------------------------
//  Interazione e collisioni
// ----------------------------------------------------------------------------
struct Hot { const char* name; V p; float reach; };
static const Hot HOT[] = {
    {"secchio di cenere",        {1.6f, .30f, -4.4f},  2.0f},   // 0
    {"scrigno",                  {-.2f, 1.05f, 0.f},   2.3f},   // 1
    {"scheletro",                {2.0f, 1.0f, 0.f},    2.8f},   // 2
    {"armatura sulla rastrelliera", {-4.6f, .9f, 0.f}, 2.8f},   // 3
    {"elmo a terra",             {-4.0f, .15f, .9f},   2.6f},   // 4
    {"spada",                    {4.47f, 2.7f, 0.f},   3.2f},   // 5
    {"scudo",                    {-4.95f, 2.0f, 0.f},  3.5f},   // 6
    {"porta nord",               {0.f, 1.3f, -5.f},    2.6f},   // 7
    {"porta sud",                {0.f, 1.3f, 5.f},     2.6f},   // 8
    {"porta est",                {5.f, 1.3f, 3.4f},    2.6f},   // 9
    {"porta ovest",              {-5.f, 1.3f, -3.4f},  2.6f},   // 10
    {"ritratto sopra la porta nord", {0.f, 3.4f, -5.f},  6.f},  // 11
    {"ritratto sopra la porta sud",  {0.f, 3.4f, 5.f},   6.f},  // 12
    {"ritratto sopra la porta est",  {5.f, 3.4f, 3.4f},  6.f},  // 13
    {"ritratto sopra la porta ovest",{-5.f, 3.4f, -3.4f},6.f},  // 14
};                                                              // 15..18: torce (dinamiche)
static const int NHOT = (int)(sizeof(HOT) / sizeof(HOT[0]));

static std::string interact(int id) {
    if (id >= 15 && id < 19) {                                  // torcia / supporto della parete i
        int i = id - 15;
        if (g_wt[i].present) {
            Item it{2, g_wt[i].lit};
            if (!giveItem(it, true)) return giveFailMsg();
            g_wt[i].present = false;
            return it.lit ? "Stacchi la torcia accesa dal supporto." : "Stacchi la torcia spenta dal supporto.";
        }
        int h = handWith(2);
        if (h >= 0) { g_wt[i] = {true, g_hand[h].lit}; g_hand[h] = Item(); return "Appendi la torcia al supporto di ferro."; }
        return "Un supporto di ferro per torce, vuoto.";
    }
    if (id == 5 || id == 6) {                                   // spada e scudo: prelevabili e riappendibili
        bool sw = id == 5; bool& onWall = sw ? g_swordOnWall : g_shieldOnWall; int type = sw ? 3 : 4;
        if (onWall) {
            if (!giveItem(Item{type, false}, true)) return giveFailMsg();
            onWall = false;
            return sw ? "Sollevi la spada dai ganci: pesante, ma ben bilanciata." : "Stacchi lo scudo dalla parete: il legno è ancora solido.";
        }
        int h = handWith(type);
        if (h >= 0) { g_hand[h] = Item(); onWall = true; return sw ? "Riponi la spada sui ganci sopra il camino." : "Riappendi lo scudo alla parete."; }
        return sw ? "Due ganci vuoti sopra il camino." : "Due ganci vuoti sulla parete.";
    }
    switch (id) {
        case 2: return "Un cavaliere dimenticato, ancora seduto a capotavola. Le ossa pesano circa " + fmtKg(SKELETON_KG) + " kg (" + fmtKg(SKELETON_KG + SKELETON_HELM_KG) + " con l'elmo): troppo fragile e pesante da spostare.";
        case 3: return "Corazza, gambali, bracciali, spalline e stivali: l'armatura è completa, manca solo chi la indossi.";
        case 7: case 8: case 9: case 10:
            return hasKey() ? "Porta sprangata dall'esterno: la chiave di bronzo non è per questa serratura."
                            : "Una pesante porta di quercia, chiusa. Non cede.";
        case 11: return "Ottone il Conquistatore, primo signore del castello (la corona).";
        case 12: return "Ser Guelfo il Cavaliere Nero, in elmo e sguardo severo.";
        case 13: return "Lord Baldovino il Barbuto, che costruì questa sala segreta.";
        case 14: return "Lord Rodrigo il Mercante, con un cappello a tesa larga.";
    }
    return "";
}

struct AABB { float x0, x1, z0, z1; };
static const AABB SOLID[] = {
    {-1.55f, 1.55f, -.70f, .70f},    // tavolo
    {1.75f, 2.50f, -.40f, .40f},     // sedia + scheletro
    {3.85f, 5.00f, -1.45f, 1.45f},   // camino
    {-5.00f, -4.35f, -1.30f, 1.30f}, // rastrelliera
};
static const float PR = .25f, LIM = 5.f - .35f;
static bool blocked(float x, float z) {
    if (x < -LIM || x > LIM || z < -LIM || z > LIM) return true;
    for (int i = 0; i < (int)(sizeof(SOLID) / sizeof(SOLID[0])); i++) {
        if (i == 0 && g_tableState == 2) continue;                       // il tavolo è bruciato: non blocca più
        if (i == 3 && g_rackState == 2) continue;                        // idem la rastrelliera
        const AABB& b = SOLID[i];
        if (x > b.x0 - PR && x < b.x1 + PR && z > b.z0 - PR && z < b.z1 + PR) return true;
    }
    return false;
}


// ----------------------------------------------------------------------------
//  Riquadro di testo in basso (SDL_ttf -> texture OpenGL)
// ----------------------------------------------------------------------------
static std::string findFont() {
    auto exists = [](const std::string& p) { FILE* f = fopen(p.c_str(), "rb"); if (f) fclose(f); return f != nullptr; };
    if (const char* e = getenv("FONT")) if (exists(e)) return e;
    const char* names[] = {"Book Antiqua", "Palatino Linotype", "Palatino", "P052", "TeX Gyre Pagella", "Palladio"};
    for (const char* n : names) {            // fontconfig: accetta solo se la famiglia trovata è davvero "palatina"
        std::string cmd = std::string("fc-match -f '%{family}|%{file}' '") + n + "' 2>/dev/null";
        FILE* p = popen(cmd.c_str(), "r"); if (!p) continue;
        char buf[1024] = {0}; size_t k = fread(buf, 1, sizeof(buf) - 1, p); pclose(p); buf[k] = 0;
        std::string r = buf, fam = r.substr(0, r.find('|')), file = r.find('|') == std::string::npos ? "" : r.substr(r.find('|') + 1);
        for (auto& c : fam) c = (char)tolower((unsigned char)c);
        for (const char* key : {"antiqua", "palatino", "p052", "pagella", "palladio"})
            if (fam.find(key) != std::string::npos && exists(file)) return file;
    }
    const char* paths[] = {
        "/usr/share/fonts/urw-base35/P052-Roman.otf", "/usr/share/fonts/opentype/urw-base35/P052-Roman.otf",
        "/usr/share/fonts/OTF/texgyrepagella-regular.otf", "/usr/share/fonts/opentype/tex-gyre/texgyrepagella-regular.otf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", "/usr/share/fonts/TTF/DejaVuSerif.ttf",
        "/usr/share/fonts/dejavu/DejaVuSerif.ttf"};
    for (const char* p : paths) if (exists(p)) return p;
    return "";
}

// Testo bianco -> texture OpenGL, con cache (le stringhe sono poche e ripetute)
struct TextCache {
    struct E { GLuint tex = 0; int w = 0, h = 0; };
    TTF_Font* font = nullptr;
    std::map<std::string, E> m;

    void init() {
        if (TTF_Init() != 0) { fprintf(stderr, "SDL_ttf: %s\n", TTF_GetError()); return; }
        std::string path = findFont();
        if (path.empty()) { fprintf(stderr, "Nessun font trovato: i messaggi andranno su terminale.\n"); return; }
        // corpo 14 pt a 96 dpi = 18,67 px (SDL_ttf usa i punti a 72 dpi, quindi 14*96/72 ≈ 19)
        font = TTF_OpenFont(path.c_str(), 19);
        if (!font) fprintf(stderr, "Font %s: %s\n", path.c_str(), TTF_GetError());
        else fprintf(stderr, "Font: %s\n", path.c_str());
    }
    E* get(const std::string& txt, int wrap) {
        if (!font || txt.empty()) return nullptr;
        std::string key = std::to_string(wrap) + "|" + txt;
        auto it = m.find(key);
        if (it != m.end()) return it->second.tex ? &it->second : nullptr;
        E e; SDL_Color white{255, 255, 255, 255};
        std::string shaped;                                   // il font P052 fonde "ce" in una legatura "œ" (in certi contesti): uno spazio sottile (U+200A) la impedisce
        for (size_t i = 0; i < txt.size(); i++) { shaped += txt[i]; if (txt[i] == 'c' && i + 1 < txt.size() && txt[i + 1] == 'e') shaped += "\xE2\x80\x8A"; }
        SDL_Surface* s0 = TTF_RenderUTF8_Blended_Wrapped(font, shaped.c_str(), white, (Uint32)wrap);
        if (s0) {
            SDL_Surface* s1 = SDL_ConvertSurfaceFormat(s0, SDL_PIXELFORMAT_ABGR8888, 0);   // byte R,G,B,A
            SDL_FreeSurface(s0);
            if (s1) {
                glGenTextures(1, &e.tex); glBindTexture(GL_TEXTURE_2D, e.tex);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, s1->pitch / 4);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s1->w, s1->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s1->pixels);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                e.w = s1->w; e.h = s1->h; SDL_FreeSurface(s1);
            }
        }
        E& ref = m[key] = e;
        return ref.tex ? &ref : nullptr;
    }
    void draw(const std::string& txt, float x, float y, int wrap = 0) {   // origine in alto a sinistra
        E* e = get(txt, wrap); if (!e) return;
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, e->tex); glColor4f(1, 1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(x, y); glTexCoord2f(1, 0); glVertex2f(x + e->w, y);
        glTexCoord2f(1, 1); glVertex2f(x + e->w, y + e->h); glTexCoord2f(0, 1); glVertex2f(x, y + e->h);
        glEnd(); glDisable(GL_TEXTURE_2D);
    }
    void panel(const std::string& txt, int w, int h);   // riquadro in basso
    void panelAt(const std::string& txt, float x, float y);   // riquadro in un punto qualsiasi
    void shutdown() {
        for (auto& kv : m) if (kv.second.tex) glDeleteTextures(1, &kv.second.tex);
        if (font) TTF_CloseFont(font);
        if (TTF_WasInit()) TTF_Quit();
    }
};

static void fillRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h); glEnd();
}
static void strokeRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_LINE_LOOP); glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h); glEnd();
}

void TextCache::panel(const std::string& txt, int w, int h) {
    const int pad = 16, margin = 28;
    E* e = get(txt, std::max(100, std::min(w - 60, 960) - 2 * pad));
    if (!e) return;
    float pw = (float)(e->w + 2 * pad), ph = (float)(e->h + 2 * pad), x0 = (w - pw) / 2.f, y0 = h - ph - margin;
    fillRect(x0, y0, pw, ph, 0, 0, 0, .70f);                  // sfondo nero semitrasparente
    strokeRect(x0, y0, pw, ph, 1, 1, 1, .28f);
    draw(txt, x0 + pad, y0 + pad, std::max(100, std::min(w - 60, 960) - 2 * pad));
}
void TextCache::panelAt(const std::string& txt, float x, float y) {
    const int pad = 14;
    E* e = get(txt, 0);
    if (!e) return;
    fillRect(x, y, (float)(e->w + 2 * pad), (float)(e->h + 2 * pad), 0, 0, 0, .70f);
    strokeRect(x, y, (float)(e->w + 2 * pad), (float)(e->h + 2 * pad), 1, 1, 1, .28f);
    draw(txt, x + pad, y + pad, 0);
}

// ---- figura umana in wireframe (per l'inventario) ----
static V vadd(V a, V b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static V vmul(V a, float k) { return {a.x * k, a.y * k, a.z * k}; }
static V vcross(V a, V b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static V vnorm(V a) { float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); return l > 1e-6f ? vmul(a, 1 / l) : V{0, 1, 0}; }
static void limb(V a, V b, float r0, float r1, int n = 8) {       // tronco di cono tra due punti qualsiasi
    V d = vnorm({b.x - a.x, b.y - a.y, b.z - a.z});
    V p1 = vnorm(vcross(d, fabsf(d.y) < .9f ? V{0, 1, 0} : V{1, 0, 0})), p2 = vcross(d, p1);
    arc(a, vmul(p1, r0), vmul(p2, r0), 0, 2 * PI, n); arc(b, vmul(p1, r1), vmul(p2, r1), 0, 2 * PI, n);
    glBegin(GL_LINES);
    for (int i = 0; i < n; i++) {
        float t = 2 * PI * i / n; V u = vadd(vmul(p1, cosf(t)), vmul(p2, sinf(t)));
        V pa = vadd(a, vmul(u, r0)), pb = vadd(b, vmul(u, r1));
        glVertex3f(pa.x, pa.y, pa.z); glVertex3f(pb.x, pb.y, pb.z);
    }
    glEnd();
}

// Uomo alto ~1,75 (piedi nell'origine, fronte verso +z): tunica con cintura, calzamaglia aderente, sandali.
static void drawMan() {
    const float skin[3] = {.96f, .82f, .70f};
    col(skin[0], skin[1], skin[2]);                                   // testa e collo
    {
        PM _; glTranslatef(0, 1.62f, 0); sphere(.095f, .115f, .105f, 4, 8);
        line(-.045f, .02f, .099f, -.02f, .02f, .104f); line(.02f, .02f, .104f, .045f, .02f, .099f);   // occhi
        strip({{0, .015f, .105f}, {0, -.03f, .12f}, {.016f, -.035f, .108f}});                         // naso
        line(-.025f, -.065f, .098f, .025f, -.065f, .098f);                                            // bocca
    }
    limb({0, 1.455f, 0}, {0, 1.53f, 0}, .045f, .04f, 8);
    col(.50f, .34f, .20f); { PM _; glTranslatef(0, 1.64f, -.01f); dome(.1f, .115f, .112f, .1f, PI / 2, 2, 8); }   // capelli

    col(.92f, .76f, .40f);                                            // tunica (anelli sovrapposti = campanatura)
    const int NR = 7;
    const float ys[NR] = {1.44f, 1.34f, 1.20f, 1.06f, .94f, .78f, .62f};
    const float rx[NR] = {.19f, .20f, .17f, .15f, .17f, .20f, .23f}, rz[NR] = {.10f, .115f, .105f, .095f, .11f, .12f, .13f};
    for (int i = 0; i < NR; i++) ringXZ({0, ys[i], 0}, rx[i], rz[i], 16);
    glBegin(GL_LINES);
    for (int k = 0; k < 12; k++) {
        float t = 2 * PI * k / 12, c = cosf(t), sn = sinf(t);
        for (int i = 0; i < NR - 1; i++) { glVertex3f(rx[i] * c, ys[i], rz[i] * sn); glVertex3f(rx[i + 1] * c, ys[i + 1], rz[i + 1] * sn); }
    }
    glEnd();
    strip({{-.05f, 1.44f, .10f}, {0, 1.32f, .116f}, {.05f, 1.44f, .10f}});    // scollo a V
    for (int sd = -1; sd <= 1; sd += 2) {                                      // maniche e braccia
        col(.92f, .76f, .40f);
        limb({sd * .20f, 1.40f, 0}, {sd * .27f, 1.12f, .02f}, .06f, .052f, 8);
        limb({sd * .27f, 1.12f, .02f}, {sd * .29f, .93f, .07f}, .052f, .045f, 8);
        col(skin[0], skin[1], skin[2]);
        limb({sd * .29f, .93f, .07f}, {sd * .30f, .80f, .10f}, .035f, .028f, 6);
        { PM _; glTranslatef(sd * .305f, .755f, .11f); sphere(.03f, .045f, .025f, 3, 6); }
    }
    col(.68f, .40f, .15f);                                                     // cintura in vita
    ringXZ({0, 1.03f, 0}, .157f, .108f, 16); ringXZ({0, 1.07f, 0}, .155f, .106f, 16);
    rect(-.025f, 1.02f, .025f, 1.08f, .114f);
    strip({{.03f, 1.03f, .113f}, {.035f, .86f, .120f}, {.065f, .86f, .120f}, {.055f, 1.03f, .113f}});

    for (int sd = -1; sd <= 1; sd += 2) {                                      // pantaloni aderenti
        col(.50f, .64f, .92f);
        limb({sd * .085f, .92f, 0}, {sd * .080f, .50f, .02f}, .085f, .060f, 10);
        limb({sd * .080f, .50f, .02f}, {sd * .075f, .12f, 0}, .060f, .040f, 10);
        col(.84f, .54f, .22f);                                                 // sandali: suola, cinghie, laccio alla caviglia
        float x = sd * .075f;
        const float sx[8] = {-.040f, .040f, .045f, .040f, .020f, -.020f, -.040f, -.045f};
        const float sz[8] = {-.06f, -.06f, .0f, .15f, .21f, .21f, .15f, .0f};
        for (float y : {.03f, .0f}) { glBegin(GL_LINE_LOOP); for (int i = 0; i < 8; i++) glVertex3f(x + sx[i], y, sz[i]); glEnd(); }
        glBegin(GL_LINES); for (int i = 0; i < 8; i++) { glVertex3f(x + sx[i], 0, sz[i]); glVertex3f(x + sx[i], .03f, sz[i]); } glEnd();
        for (float z : {.02f, .08f, .14f}) arc({x, .03f, z}, {.045f, 0, 0}, {0, .045f, 0}, 0, PI, 8);
        ringXZ({x, .105f, -.005f}, .042f, .042f, 8);
        line(x, .03f, -.06f, x, .105f, -.04f); line(x, .075f, .02f, x, .105f, 0);
    }
    // armatura indossata
    col(.70f, .78f, .92f);
    if (g_equip[0]) { PM _; glTranslatef(0, 1.665f, 0); drawHelm(.125f); }
    if (g_equip[1]) { PM _; glTranslatef(0, 1.03f, 0); glScalef(1, 1, .62f); cone(.165f, .215f, .40f, 12, 12); }
    for (int sd = -1; sd <= 1; sd += 2) {
        int a = sd < 0 ? 0 : 1;
        if (g_equip[2 + a]) { PM _; glTranslatef(sd * .225f, 1.40f, 0); dome(.10f, .07f, .10f, 0, PI / 2, 3, 8); }
        if (g_equip[4 + a]) limb({sd * .29f, .93f, .07f}, {sd * .30f, .80f, .10f}, .05f, .042f, 8);
        if (g_equip[6 + a]) limb({sd * .08f, .50f, .02f}, {sd * .075f, .15f, 0}, .072f, .052f, 8);
        if (g_equip[8 + a]) { limb({sd * .075f, .20f, 0}, {sd * .075f, .03f, .01f}, .058f, .062f, 8); boxAt(sd * .075f, .045f, .07f, .095f, .085f, .28f); }
    }
}

// ---- inventario ----
// slot: 0..8 griglia, 9 = mano sinistra, 10 = mano destra
struct InvLayout { float slot, gap, pad, titleH, px, py, pw, ph, hx, hy0, hy1, gx, gy, fx, fy, fw, fh; };
static InvLayout invLayout(int w, int h) {
    InvLayout L; L.slot = 88; L.gap = 10; L.pad = 26; L.titleH = 34;
    float grid = 3 * L.slot + 2 * L.gap, handGap = 40, figGap = 30; L.fw = 210; L.fh = grid;
    L.pw = L.pad * 2 + L.slot + handGap + grid + figGap + L.fw; L.ph = L.pad * 2 + L.titleH + grid + 182;   // 182 = statistiche sotto la figura e righe del peso
    L.px = (w - L.pw) / 2; L.py = (h - L.ph) / 2 - 36;
    L.hx = L.px + L.pad; L.gx = L.hx + L.slot + handGap; L.gy = L.py + L.pad + L.titleH;
    L.hy0 = L.gy + 36; L.hy1 = L.hy0 + L.slot + 44;
    L.fx = L.gx + grid + figGap; L.fy = L.gy;
    return L;
}
static void slotRect(const InvLayout& L, int i, float& x, float& y) {
    if (i == 9) { x = L.hx; y = L.hy0; return; }
    if (i == 10) { x = L.hx; y = L.hy1; return; }
    if (i >= 11) {                                              // slot di equipaggiamento ai lati della figura
        int k = i - 11; float sz = 32;
        x = (k & 1) ? L.fx + L.fw - 6 - sz : L.fx + 6; y = L.fy + 8 + (k / 2) * ((L.fh - 16 - sz) / 4);
        return;
    }
    x = L.gx + (i % 3) * (L.slot + L.gap); y = L.gy + (i / 3) * (L.slot + L.gap);
}
static int invHit(const InvLayout& L, float mx, float my) {
    for (int i = 0; i < 21; i++) { float x, y, sz = i >= 11 ? 32.f : L.slot; slotRect(L, i, x, y); if (mx >= x && mx <= x + sz && my >= y && my <= y + sz) return i; }
    return -1;
}
static bool figHit(const InvLayout& L, float mx, float my) { return mx >= L.fx && mx <= L.fx + L.fw && my >= L.fy && my <= L.fy + L.fh; }
static void slotAction(int i, int hand) {                       // hand: 0 = sinistra, 1 = destra
    if (i == 9 || i == 10) {                                    // clic su una mano: riponi nell'inventario
        Item& hh = g_hand[i - 9];
        if (hh.type) for (auto& s : g_inv) if (!s.type) { s = hh; hh = Item(); break; }
    } else if (i >= 0 && i < 9) std::swap(g_hand[hand], g_inv[i]);   // prende, ripone o scambia
}
static bool autoEquip(Item& it) {              // indossa il pezzo nel primo slot libero del suo tipo
    if (!isArmor(it.type)) return false;
    for (int i = 0; i < 10; i++) if (SLOT_TYPE[i] == it.type && !g_equip[i]) { g_equip[i] = true; it = Item(); return true; }
    return false;
}
static std::string equipClick(int idx, int hand) {   // clic su uno slot di equipaggiamento
    if (g_equip[idx]) {                                     // toglie il pezzo: in mano (se libera) o nell'inventario
        Item it{SLOT_TYPE[idx], false};
        if (!g_hand[hand].type) g_hand[hand] = it;
        else {
            bool ok = false;
            for (auto& sl : g_inv) if (!sl.type) { sl = it; ok = true; break; }
            if (!ok) return "Mani e inventario pieni: non puoi togliere il pezzo.";
        }
        g_equip[idx] = false;
        return itemName(it) + " tolto.";
    }
    if (g_hand[hand].type == SLOT_TYPE[idx]) { Item it = g_hand[hand]; g_equip[idx] = true; g_hand[hand] = Item(); return itemName(it) + " indossato."; }
    return "Tieni in mano il pezzo giusto (clic: mano destra · clic destro: sinistra) e poi clicca lo slot per indossarlo.";
}
static void drawItemModel(int type) {          // modelli centrati e normalizzati (~0,45 u) per icone e mano
    PM _;
    switch (type) {
        case 6: glScalef(3.5f, 3.5f, 3.5f); drawGemModel(); break;
        case 7: glScalef(1.7f, 1.7f, 1.7f); glTranslatef(-.06f, -.02f, -.03f); drawCoinsModel(); break;      // le monete: icona più piccola
        case 8: glScalef(1.05f, 1.05f, 1.05f); glTranslatef(0, -.02f, 0); drawBookModel(); break;
        case 10: glScalef(1.7f, 1.7f, 1.7f); col(.70f, .78f, .92f); drawHelm(.13f); break;
        case 11: glScalef(.9f, .9f, .9f); glTranslatef(0, -.25f, 0); drawCuirassModel(); break;
        case 12: glScalef(1.6f, 1.6f, 1.6f); glTranslatef(0, -.04f, 0); drawPauldronModel(); break;
        case 13: glTranslatef(0, -.225f, 0); drawGreaveModel(); break;
        case 14: glScalef(1.5f, 1.5f, 1.5f); glTranslatef(0, -.15f, 0); drawVambraceModel(); break;
        case 15: glScalef(1.4f, 1.4f, 1.4f); glTranslatef(0, -.15f, 0); drawBootModel(); break;
    }
}
static void drawItemIcon(const Item& it, float cx, float cy, float s) {
    if (!it.type) return;
    PM _; glTranslatef(cx, cy, 0);
    switch (it.type) {
        case 1:
            glRotatef(35, 0, 0, 1); glScalef(s * 1.2f, -s * 1.2f, s); glTranslatef(-.24f, 0, 0);
            col(.95f, .70f, .20f); drawKey(); break;
        case 2:
            glRotatef(22, 0, 0, 1); glScalef(s * .80f, -s * .80f, s * .80f); glRotatef(28, 1, 0, 0); glRotatef(35, 0, 1, 0); glTranslatef(0, -.22f, 0);
            drawTorchBody(it.lit); break;
        case 3:
            glRotatef(-42, 0, 0, 1); glScalef(s * .40f, -s * .40f, s * .40f); glTranslatef(0, -2.7f, -.54f);
            drawSword(); break;
        case 4:
            glScalef(s * .55f, -s * .55f, s * .55f); glRotatef(20, 0, 1, 0); glTranslatef(0, -2.0f, -.045f);
            drawShield(); break;
        case 5:
            glScalef(s * .95f, -s * .95f, s * .95f); glRotatef(22, 1, 0, 0); glTranslatef(0, -.30f, 0);
            drawBucketBody(); break;
        case 6: case 7: case 8: case 10: case 11: case 12: case 13: case 14: case 15:
            glScalef(s * 1.5f, -s * 1.5f, s * 1.5f); glRotatef(22, 1, 0, 0); glRotatef(30, 0, 1, 0); drawItemModel(it.type); break;
    }
}
static const int STAT_MOD[5] = {2, -1, -1, 0, 2};              // per ogni pezzo d'armatura indossato: Forza e Vita +2, Destrezza e Magia -1
// Bonus/malus: true = si applicano per ogni pezzo indossato; false = una sola volta se si indossa almeno un pezzo
static const bool ARMOR_BONUS_PER_PIECE = false;
static int armorCount() { int n = 0; for (bool b : g_equip) n += b ? 1 : 0; return n; }
static int g_stats[5] = {0, 0, 0, 0, 0};
static const char* STAT_NAMES[5] = {"Forza", "Destrezza", "Magia", "Carisma", "Vita"};
static int effStat(int i) { int n = ARMOR_BONUS_PER_PIECE ? armorCount() : (armorCount() > 0 ? 1 : 0); return std::max(0, g_stats[i] + STAT_MOD[i] * n); }
static int hpMax() { return std::max(1, effStat(4)); }                      // la Vita (con i modificatori) è il massimo dei punti vita
static void drawInventory(TextCache& tc, int w, int h, int hover) {
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
        if (g_equip[i]) drawItemIcon(Item{SLOT_TYPE[i], false}, x + 16, y + 16, 32);
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

static void drawHourglass(TextCache& tc, float cx, float cy, float h, float frac) {   // clessidra animata (sabbia che scende)
    float w = h * .52f, hh = h / 2;
    col(.85f, .85f, .95f);
    line(cx - w / 2 - 4, cy - hh, 0, cx + w / 2 + 4, cy - hh, 0); line(cx - w / 2 - 4, cy + hh, 0, cx + w / 2 + 4, cy + hh, 0);
    strip({{cx - w / 2, cy - hh, 0}, {cx - 3, cy, 0}, {cx - w / 2, cy + hh, 0}});
    strip({{cx + w / 2, cy - hh, 0}, {cx + 3, cy, 0}, {cx + w / 2, cy + hh, 0}});
    float ws = 6 + (w - 6) * frac, ys = cy - frac * hh, yb = cy + hh - (1 - frac) * hh, wb = 6 + (w - 6) * (1 - frac);
    glColor4f(.95f, .80f, .40f, .85f);
    glBegin(GL_QUADS);                                                    // sabbia nel bulbo superiore
    glVertex2f(cx - ws / 2, ys); glVertex2f(cx + ws / 2, ys); glVertex2f(cx + 3, cy); glVertex2f(cx - 3, cy);
    glEnd();
    glBegin(GL_QUADS);                                                    // sabbia accumulata in basso
    glVertex2f(cx - w / 2, cy + hh); glVertex2f(cx + w / 2, cy + hh); glVertex2f(cx + wb / 2, yb); glVertex2f(cx - wb / 2, yb);
    glEnd();
    if (frac > 0.f && yb > cy + 4) {                                      // filo di sabbia che cade
        col(.95f, .80f, .40f);
        for (int k = 0; k < 4; k++) { float yy = cy + fmodf(g_time * 90.f + k * 17.f, std::max(8.f, yb - cy)); line(cx, yy, 0, cx, std::min(yy + 7.f, yb), 0); }
    }
    tc.draw(std::to_string((int)ceilf(frac * 10.f)) + " s", cx - 14, cy + hh + 8);
}

// ---- minigioco dei dadi ----
struct Die { V start, rest, axis; float spinDeg, yaw; int value; };
struct DiceGame {
    int skill = 0, throwNo = 0, scores[5][3] = {};
    bool rolling = false, finished = false, shown = false;
    float t = 0; const float TT = 1.9f;
    Die die[2];
    std::mt19937 rng{std::random_device{}()};
};
static void pips(int n, float d, float r) {
    std::vector<std::pair<int, int>> p;
    switch (n) {
        case 1: p = {{0, 0}}; break;
        case 2: p = {{-1, -1}, {1, 1}}; break;
        case 3: p = {{-1, -1}, {0, 0}, {1, 1}}; break;
        case 4: p = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}}; break;
        case 5: p = {{-1, -1}, {1, -1}, {0, 0}, {-1, 1}, {1, 1}}; break;
        default: p = {{-1, -1}, {-1, 0}, {-1, 1}, {1, -1}, {1, 0}, {1, 1}}; break;
    }
    for (auto& q : p) { ringXY({q.first * d, q.second * d, 0}, r, r, 8); ringXY({q.first * d, q.second * d, 0}, r * .5f, r * .5f, 6); }
}
static void drawDieModel(float s) {          // facce: +Y=1, -Y=6, +X=2, -X=5, +Z=3, -Z=4 (opposte sommano 7)
    col(.95f, .95f, .88f); box(2 * s, 2 * s, 2 * s);
    col(1.f, .35f, .30f);
    for (int f = 0; f < 6; f++) {
        PM _;
        switch (f) {
            case 0: break;                                   // +Z -> 3
            case 1: glRotatef(180, 0, 1, 0); break;          // -Z -> 4
            case 2: glRotatef(90, 0, 1, 0); break;           // +X -> 2
            case 3: glRotatef(-90, 0, 1, 0); break;          // -X -> 5
            case 4: glRotatef(-90, 1, 0, 0); break;          // +Y -> 1
            case 5: glRotatef(90, 1, 0, 0); break;           // -Y -> 6
        }
        static const int val[6] = {3, 4, 2, 5, 1, 6};
        glTranslatef(0, 0, s + .0015f); pips(val[f], s * .5f, s * .15f);
    }
}
static void faceUp(int v) {                  // ruota il dado in modo che la faccia v guardi in alto
    switch (v) {
        case 6: glRotatef(180, 1, 0, 0); break; case 2: glRotatef(90, 0, 0, 1); break; case 5: glRotatef(-90, 0, 0, 1); break;
        case 3: glRotatef(-90, 1, 0, 0); break; case 4: glRotatef(90, 1, 0, 0); break; default: break;
    }
}
static void drawDie(const Die& d, float u) {
    const float s = .05f;
    u = std::max(0.f, std::min(1.f, u));
    float e = 1 - (1 - u) * (1 - u), ry = d.rest.y, y;       // scivola e rallenta; rimbalza tre volte e si ferma
    if (u < .40f) { float k = u / .40f; y = ry + (d.start.y - ry) * (1 - k * k); }
    else if (u < .65f) y = ry + .09f * sinf(PI * (u - .40f) / .25f);
    else if (u < .82f) y = ry + .035f * sinf(PI * (u - .65f) / .17f);
    else if (u < .93f) y = ry + .012f * sinf(PI * (u - .82f) / .11f);
    else y = ry;
    float x = d.start.x + (d.rest.x - d.start.x) * e, z = d.start.z + (d.rest.z - d.start.z) * e;
    float ang = u >= 1.f ? 0.f : d.spinDeg * powf(1 - u, 1.8f);     // la rotazione si esaurisce esattamente sulla faccia scelta
    col(.50f, .44f, .36f);                                           // ombra sul tavolo
    float sh = s * (1.1f + (y - ry) * 2.f);
    strip({{x - sh, .902f, z - sh}, {x + sh, .902f, z - sh}, {x + sh, .902f, z + sh}, {x - sh, .902f, z + sh}}, true);
    PM _; glTranslatef(x, y, z);
    glRotatef(ang, d.axis.x, d.axis.y, d.axis.z); glRotatef(d.yaw, 0, 1, 0); faceUp(d.value);
    drawDieModel(s);
}
static void startRoll(DiceGame& g) {
    auto R = [&](float a, float b) { return std::uniform_real_distribution<float>(a, b)(g.rng); };
    for (int i = 0; i < 2; i++) {
        Die& d = g.die[i];
        d.value = std::uniform_int_distribution<int>(1, 6)(g.rng);
        d.rest = {i == 0 ? R(-.42f, -.20f) : R(.02f, .30f), .95f, R(.28f, .50f)};
        d.start = {d.rest.x + R(-.25f, .25f), 1.25f, .95f};
        V ax{R(-1, 1), R(-1, 1), R(-1, 1)}; d.axis = vnorm(ax);
        d.spinDeg = R(900.f, 1500.f); d.yaw = R(0.f, 360.f);
    }
    g.rolling = true; g.shown = true; g.t = 0;
}
static std::string scoreboard(const DiceGame& g) {
    std::string s = "Prove delle caratteristiche (3 lanci, vale il migliore)\n";
    for (int i = 0; i < 5; i++) {
        s += std::string(STAT_NAMES[i]) + ":  ";
        for (int j = 0; j < 3; j++) {
            s += (i < g.skill || (i == g.skill && j < g.throwNo)) ? std::to_string(g.scores[i][j]) : "–";
            if (j < 2) s += " · ";
        }
        if (i < g.skill) s += "   →  " + std::to_string(g_stats[i]);
        if (i < 4) s += "\n";
    }
    return s;
}

// ---- luci: le torce accese (e quella in mano) illuminano le linee vicine ----
// Luce solo "ambientale" con attenuazione quadratica: il colore di ogni vertice viene scalato dalla vicinanza alle fiamme.
static void setLights(const std::vector<V>& pos) {
    GLfloat gam[4] = {.26f, .30f, .42f, 1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, gam);
    for (int i = 0; i < 8; i++) {
        if (i >= (int)pos.size()) { glDisable(GL_LIGHT0 + i); continue; }
        float fl = 1 + .10f * sinf(g_time * 13 + i * 2.1f) + .05f * sinf(g_time * 31 + i);
        GLfloat amb[4] = {1.9f * fl, 1.15f * fl, .48f * fl, 1}, off[4] = {0, 0, 0, 1};
        GLfloat p[4] = {pos[i].x, pos[i].y, pos[i].z, 1};
        glEnable(GL_LIGHT0 + i);
        glLightfv(GL_LIGHT0 + i, GL_POSITION, p);
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, amb); glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, off); glLightfv(GL_LIGHT0 + i, GL_SPECULAR, off);
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 1.f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, .16f);
    }
}

// ----------------------------------------------------------------------------
//  Programma principale
// ----------------------------------------------------------------------------
static void lookAt(V eye, V f) {
    float fl = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z); f = {f.x / fl, f.y / fl, f.z / fl};
    V s{-f.z, 0, f.x}; float sl = sqrtf(s.x * s.x + s.z * s.z); s = {s.x / sl, 0, s.z / sl};
    V u{s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x};
    float m[16] = {s.x, u.x, -f.x, 0,  s.y, u.y, -f.y, 0,  s.z, u.z, -f.z, 0,  0, 0, 0, 1};
    glMultMatrixf(m); glTranslatef(-eye.x, -eye.y, -eye.z);
}

static void savePPM(const char* path, int w, int h) {
    std::vector<unsigned char> px((size_t)w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    FILE* f = fopen(path, "wb"); if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; y--) fwrite(&px[(size_t)y * w * 3], 1, (size_t)w * 3, f);
    fclose(f);
}

// oggetto tenuto in mano (side: -1 sinistra, +1 destra); join = 0..1 avvicina le mani al centro
static void drawHandItem(const Item& it, int side, float join) {
    if (!it.type) return;
    PM _; glLoadIdentity();
    float bx = side * .30f * (1 - .7f * join);
    switch (it.type) {
        case 2:
            glTranslatef(bx, -.38f, -.60f); glRotatef(-20, 1, 0, 0); glRotatef(side * 10.f, 0, 0, 1);
            drawTorchBody(it.lit); break;
        case 1:
            glTranslatef(bx * .93f, -.22f, -.55f); glScalef((float)side, 1, 1);
            glRotatef(70, 0, 0, 1); glRotatef(-25, 0, 1, 0); glRotatef(-60, 1, 0, 0);
            col(.95f, .70f, .20f); drawKey(); break;
        case 3:   // spada: impugnata con la punta in alto e in avanti
            glTranslatef(bx * .95f, -.42f, -.62f); glScalef((float)side, 1, 1);
            glRotatef(-32, 1, 0, 0); glRotatef(-8, 0, 0, 1); glScalef(.5f, .5f, .5f); glRotatef(90, 0, 0, 1);
            glTranslatef(.8f, -2.7f, -.54f); drawSword(); break;
        case 4:   // scudo: imbracciato, rivolto in avanti
            glTranslatef(bx * 1.05f, -.30f, -.72f); glScalef((float)side, 1, 1);
            glRotatef(-18.f, 0, 1, 0); glScalef(.27f, .27f, .27f); glTranslatef(0, -2.0f, 0);
            drawShield(); break;
        case 5:   // secchio: portato a mano, un po' più in basso
            glTranslatef(bx * .9f, -.62f, -.78f); glRotatef(15, 1, 0, 0); glScalef(.85f, .85f, .85f);
            drawBucketBody(); break;
        case 6: case 7: case 8: case 10: case 11: case 12: case 13: case 14: case 15:   // gemma, monete, libro, pezzi d'armatura
            glTranslatef(bx * .9f, -.36f, -.70f); glRotatef(15, 1, 0, 0); glRotatef(side * 20.f, 0, 1, 0); glScalef(.6f, .6f, .6f);
            drawItemModel(it.type); break;
    }
}

struct HotD { std::string label; V p; float reach; int kind, idx; };   // kind: 0 statico, 1 sciolto, 2 torcia a muro, 3 spada, 4 scudo
static bool movableHot(const HotD& h) {      // kind 5/6: contenuto dello scrigno; 7: pezzi sulla rastrelliera
    return (h.kind == 1 && g_loose[h.idx].type != LT_ASH) || h.kind == 5 || h.kind == 6 || (h.kind == 7 && g_rackHas[h.idx]) ||
           (h.kind == 2 && g_wt[h.idx].present) || (h.kind == 3 && g_swordOnWall) || (h.kind == 4 && g_shieldOnWall);
}
static const V DOOR_C[4] = {{0, 0, -5}, {0, 0, 5}, {5, 0, 3.4f}, {-5, 0, -3.4f}};       // centro delle porte sul muro (x,z)
static const V DOOR_N[4] = {{0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0}};                // normali verso l'interno della sala
static bool validDrop(float x, float z) {                                  // dove si può lasciar cadere un oggetto
    if (fabsf(x) > 4.55f || fabsf(z) > 4.55f) return false;
    if (x > 3.9f && x < 4.85f && fabsf(z) < .9f) return true;              // dentro il focolare (si può buttare nel fuoco)
    if (x > 3.8f && fabsf(z) < 1.5f) return false;                         // resto del camino
    if (g_rackState != 2 && x < -4.3f && fabsf(z) < 1.35f) return false;   // rastrelliera
    if (x > 1.7f && x < 2.55f && fabsf(z) < .45f) return false;            // sedia e scheletro
    return true;
}

int main(int argc, char** argv) {
    const char* shot = nullptr; bool demo = false, invOpen = false, dragging = false, skipDice = false, carryDemo = false, menuDemo = false;
    float px = -3.6f, pz = 3.6f, yaw = PI / 4, pitch = -.12f, diceShot = -1.f; bool rackBurnt = false; int hpArg = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--shot") && i + 1 < argc) {
            shot = argv[++i];
            if (i + 4 < argc && strncmp(argv[i + 1], "--", 2) != 0) {
                px = (float)atof(argv[i + 1]); pz = (float)atof(argv[i + 2]);
                yaw = (float)atof(argv[i + 3]) * PI / 180; pitch = (float)atof(argv[i + 4]) * PI / 180; i += 4;
            }
        } else if (!strcmp(argv[i], "--demo")) demo = true;
        else if (!strcmp(argv[i], "--inv")) invOpen = true;
        else if (!strcmp(argv[i], "--skipdice")) skipDice = true;
        else if (!strcmp(argv[i], "--carry")) carryDemo = true;
        else if (!strcmp(argv[i], "--menu")) menuDemo = true;
        else if (!strcmp(argv[i], "--fire")) g_fireLit = true;
        else if (!strcmp(argv[i], "--burn")) { g_tableState = 1; g_tableT = 2.f; }
        else if (!strcmp(argv[i], "--burnt")) g_tableState = 2;
        else if (!strcmp(argv[i], "--open")) g_chestOpen = true;
        else if (!strcmp(argv[i], "--worn")) { for (bool& b : g_equip) b = true; for (bool& b : g_rackHas) b = false; }
        else if (!strcmp(argv[i], "--rackburn")) { g_rackState = 1; g_rackT = 2.5f; }
        else if (!strcmp(argv[i], "--rackburnt")) rackBurnt = true;
        else if (!strcmp(argv[i], "--poison") && i + 1 < argc) g_poison = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--hp") && i + 1 < argc) hpArg = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--zap")) { g_zapT = .7f; g_zapDoor = 2; }
        else if (!strcmp(argv[i], "--dicetime") && i + 1 < argc) diceShot = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--fig") && i + 1 < argc) g_figAngle = (float)atof(argv[++i]) * PI / 180;
    }
    bool dicePhase = (!shot || diceShot >= 0) && !demo && !skipDice;     // all'avvio: minigioco dei dadi
    if (!dicePhase) { g_stats[0] = 9; g_stats[1] = 8; g_stats[2] = 7; g_stats[3] = 10; g_stats[4] = 11; }
    if (demo) {
        g_hand[1] = Item{2, true}; g_hand[0] = Item{4, false};
        g_inv[0] = Item{1, false}; g_inv[1] = Item{2, false}; g_inv[2] = Item{3, false}; g_inv[4] = Item{2, true}; g_inv[5] = Item{5, false};
        g_keyInBucket = false; g_swordOnWall = false; g_shieldOnWall = false; g_loose.erase(g_loose.begin());
    }
    if (rackBurnt) {                                  // test: rastrelliera già crollata, pezzi a terra e roventi
        g_rackState = 2;
        for (int i = 0; i < 9; i++) if (g_rackHas[i]) { Loose a{LT_ARMOR, false, V{RACK_POS[i].x + .5f, 0.f, RACK_POS[i].z}, i * .7f, 0.f, RACK_TYPE[i]}; a.hot = 10.f; g_loose.push_back(a); g_rackHas[i] = false; }
    }
    if (g_tableState == 2) for (int i = 0; i < 700; i++) updateSkeleton(.016f);   // test: braccia già assestate
    if (!dicePhase) { g_hp = hpMax(); if (hpArg > 0) g_hp = std::min(hpArg, hpMax()); }
    if (carryDemo) { g_carry = g_loose[0]; g_loose.erase(g_loose.begin()); g_carrying = true; }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_Window* win = SDL_CreateWindow("La Sala Segreta", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "Finestra: %s\n", SDL_GetError()); return 1; }
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) { fprintf(stderr, "Contesto OpenGL: %s\n", SDL_GetError()); return 1; }
    SDL_GL_SetSwapInterval(1);
    TextCache tc; tc.init();
    SDL_SetWindowTitle(win, "La Sala Segreta — WASD muoviti · mouse guarda · Shift corri · E interagisci (tieni premuto: trasporta) · I inventario · F torce · Esc esci");

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH); glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(1.4f);
    const float bg[4] = {.015f, .017f, .025f, 1.f};
    glClearColor(bg[0], bg[1], bg[2], 1);
    glFogi(GL_FOG_MODE, GL_LINEAR); glFogf(GL_FOG_START, 4.f); glFogf(GL_FOG_END, 17.f); glFogfv(GL_FOG_COLOR, bg);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);

    bool running = true, grab = !shot;
    auto applyMouse = [&]() { SDL_SetRelativeMouseMode((grab && !invOpen) ? SDL_TRUE : SDL_FALSE); };
    applyMouse();
    Uint64 last = SDL_GetPerformanceCounter();
    std::string msg, lastDice; float msgT = 0, igniteT = 0, join = 0, poisonTick = 0; bool restart = false;
    int frame = 0, lastW = 1280, lastH = 720;
    DiceGame dg;
    // menu a scelta multipla: 0 nessuno; 1 secchio a terra non frugato; 2 secchio in slot/mano non frugato
    int menu = menuDemo ? 1 : 0, menuRef = 0, menuSlot = -1, menuHand = 1, menuChoice = 0;
    bool eDown = false, eLong = false, pressPending = false, pressHeavy = false; float eTime = 0; HotD pressT{};

    // ---- azioni (lambda) ----
    auto say = [&](const std::string& m, float t = 6.f) { msg = m; msgT = t; if (!tc.font) { printf("%s\n", m.c_str()); fflush(stdout); } };
    auto hurt = [&](int n, const char* cause) { if (g_dead) return; g_hp -= n; if (g_hp <= 0) { g_hp = 0; g_dead = true; g_deathCause = cause; } };
    auto startPoison = [&]() { if (g_poison <= 0) poisonTick = 0; g_poison = 10.f; };
    auto searchBucket = [&]() {
        if (giveItem(Item{1, false}, false)) { g_keyInBucket = false; say("Frughi nella cenere fredda... e senti del metallo: una CHIAVE DI BRONZO! La metti con te."); }
        else say("Senti qualcosa sotto la cenere, ma non riesci a prenderlo. " + giveFailMsg());
    };
    auto shortLoose = [&](int k) {
        Loose o = g_loose[k];
        if (o.hot > 0) { hurt(1, "ustionato dall'armatura rovente"); say("Scotta! Il metallo è ancora rovente: -1 vita."); return; }
        switch (o.type) {
            case LT_BUCKET:
                if (g_keyInBucket) { menu = 1; menuRef = k; }
                else if (giveItem(Item{5, false}, true)) { g_loose.erase(g_loose.begin() + k); say("Raccogli il secchio, ormai vuoto di segreti."); }
                else say(giveFailMsg());
                break;
            case LT_CHEST:
                if (g_chestOpen) say((g_chestGem || g_chestCoins) ? "Lo scrigno è aperto: dentro luccica ancora qualcosa. Puntalo e premi E." : "Lo scrigno è aperto e vuoto.");
                else if (hasKey()) menu = 3;                                       // scelta multipla: hai la chiave
                else say("Lo scrigno è chiuso da un grosso lucchetto. Serve una chiave.");
                break;
            case LT_ASH: startPoison(); say("Le ceneri tossiche si alzano in una nube: ti senti male! Veleno per 10 secondi."); break;
            default: {
                Item it = itemOfLoose(o);
                if (giveItem(it, true)) { g_loose.erase(g_loose.begin() + k); say(std::string("Raccogli ") + looseName(o) + "."); }
                else say(giveFailMsg());
            }
        }
    };
    auto shortAction = [&](const HotD& t) {
        if (t.kind == 0) say(interact(t.idx));
        else if (t.kind == 1) shortLoose(t.idx);
        else if (t.kind == 2) say(interact(15 + t.idx));
        else if (t.kind == 3) say(interact(5));
        else if (t.kind == 4) say(interact(6));
        else if (t.kind == 5 || t.kind == 6) { menu = 4; menuRef = t.kind - 5; }      // contenuto dello scrigno: inventario o mano
        else if (t.kind == 7 && g_rackState == 1) { hurt(1, "ustionato dalla rastrelliera in fiamme"); say("Le fiamme della rastrelliera ti scottano: -1 vita."); }
        else if (t.kind == 7) {
            Item it{RACK_TYPE[t.idx], false};
            if (giveItem(it, true)) { g_rackHas[t.idx] = false; say(std::string("Prendi ") + armorArt(it.type) + " dalla rastrelliera."); }
            else say(giveFailMsg());
        }
    };
    auto startCarry = [&](const HotD& t) {
        Loose c{};
        if (t.kind == 1) c = g_loose[t.idx];
        else if (t.kind == 2) c = Loose{LT_TORCH, g_wt[t.idx].lit, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 3) c = Loose{LT_SWORD, false, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 4) c = Loose{LT_SHIELD, false, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 5) c = Loose{LT_GEM, false, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 6) c = Loose{LT_COINS, false, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 7) c = Loose{LT_ARMOR, false, V{0, 0, 0}, 0.f, 0.f, RACK_TYPE[t.idx]};
        else return;
        if (c.type == LT_ASH) { startPoison(); say("Le ceneri tossiche si alzano in una nube: ti senti male! Veleno per 10 secondi."); return; }
        if (c.hot > 0 || (t.kind == 7 && g_rackState == 1)) { hurt(1, "ustionato"); say("Scotta! È rovente: -1 vita."); return; }
        std::string wf = weightFail(looseKg(c));
        if (!wf.empty()) { say(wf + " Non riesci a sollevarlo."); return; }
        if (t.kind == 1) g_loose.erase(g_loose.begin() + t.idx);                    // lo toglie dalla sua posizione
        else if (t.kind == 2) g_wt[t.idx].present = false;
        else if (t.kind == 3) g_swordOnWall = false;
        else if (t.kind == 4) g_shieldOnWall = false;
        else if (t.kind == 5) g_chestGem = false;
        else if (t.kind == 6) g_chestCoins = false;
        else if (t.kind == 7) g_rackHas[t.idx] = false;
        g_carry = c; g_carrying = true;
        say(std::string("Stai trasportando ") + looseName(g_carry) + " (" + fmtKg(looseKg(g_carry)) + " kg). Rilascia E per lasciarlo cadere.", 8.f);
    };
    auto invActivate = [&](int slot, int hand, bool ctrl) {
        Item& it = slot >= 9 ? g_hand[slot - 9] : g_inv[slot];
        if (ctrl && isArmor(it.type)) { say(autoEquip(it) ? "Pezzo indossato." : "Hai già indossato tutti i pezzi di quel tipo.", 4.f); }
        else if (it.type == 5 && g_keyInBucket) { menu = 2; menuSlot = slot; menuHand = hand; }
        else slotAction(slot, hand);
    };

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = std::min(.05f, (float)(now - last) / SDL_GetPerformanceFrequency()); last = now;
        g_time += dt;
        bool ePressed = false, eReleased = false, spacePressed = false, dropPressed = false;
        if (shot && frame == 1 && !demo && !dicePhase && !carryDemo && !menuDemo) ePressed = true;   // --shot: esamina l'oggetto puntato

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                SDL_Keycode kc = e.key.keysym.sym;
                if (kc == SDLK_ESCAPE) {
                    if (menu) menu = 0;
                    else if (invOpen) { invOpen = false; dragging = false; applyMouse(); }
                    else running = false;
                }
                else if (menu && (kc == SDLK_1 || kc == SDLK_2)) menuChoice = (kc == SDLK_1) ? 1 : 2;
                else if (dicePhase) { if (kc == SDLK_SPACE || kc == SDLK_RETURN) spacePressed = true; }
                else if (g_dead) { if (kc == SDLK_r) { restart = true; running = false; } }
                else if (kc == SDLK_e) { if (SDL_GetModState() & KMOD_SHIFT) dropPressed = true; else ePressed = true; }   // Maiusc+E: lascia cadere
                else if (kc == SDLK_i && !menu) { invOpen = !invOpen; dragging = false; applyMouse(); }
                else if (kc == SDLK_TAB) { grab = !grab; applyMouse(); }
                else if (invOpen && !menu && kc >= SDLK_1 && kc <= SDLK_9)
                    invActivate((int)(kc - SDLK_1), (SDL_GetModState() & KMOD_SHIFT) ? 0 : 1, (SDL_GetModState() & KMOD_CTRL) != 0);
            } else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_e) {
                eReleased = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && invOpen && !menu) {
                int ww, wh; SDL_GetWindowSize(win, &ww, &wh);
                float mx = e.button.x * (float)lastW / ww, my = e.button.y * (float)lastH / wh;
                InvLayout L = invLayout(lastW, lastH);
                int hit = invHit(L, mx, my);
                int hnd = e.button.button == SDL_BUTTON_RIGHT ? 0 : 1;                      // clic = mano destra, clic destro = sinistra
                if (hit >= 11) say(equipClick(hit - 11, hnd), 5.f);
                else if (hit >= 0) invActivate(hit, hnd, (SDL_GetModState() & KMOD_CTRL) != 0);
                else if (e.button.button == SDL_BUTTON_LEFT && figHit(L, mx, my)) dragging = true;
            } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            } else if (e.type == SDL_MOUSEMOTION) {
                if (dragging && invOpen) g_figAngle += e.motion.xrel * .012f;
                else if (grab && !invOpen && !menu && !dicePhase) { yaw += e.motion.xrel * .0025f; pitch -= e.motion.yrel * .0025f; }
            }
        }
        // scelta nel menu a scelta multipla
        if (menuChoice && menu) {
            if (menu == 1) {
                if (menuChoice == 1) searchBucket();
                else {
                    bool ok = false; std::string wf = weightFail(itemKg(5));
                    if (wf.empty()) for (auto& sl : g_inv) if (!sl.type) { sl = Item{5, false}; g_loose.erase(g_loose.begin() + menuRef); ok = true; break; }
                    say(ok ? "Tieni il secchio nell'inventario: potrai frugarlo più tardi." : !wf.empty() ? wf : "L'inventario è pieno (tasto I).");
                }
            } else if (menu == 2) {
                if (menuChoice == 1) searchBucket();
                else if (menuSlot >= 0) slotAction(menuSlot, menuHand);
            } else if (menu == 3) {                                                    // scrigno chiuso, con la chiave
                if (menuChoice == 1) { g_chestOpen = true; say("La chiave di bronzo gira nel lucchetto: clic! Il coperchio si solleva: dentro una gemma e delle monete d'oro."); }
                else say("Lo scrigno è pesante: tieni premuto E per trasportarlo. Non entra nell'inventario.");
            } else if (menu == 4) {                                                    // contenuto: inventario o mano
                Item it{menuRef == 0 ? 6 : 7, false}; bool ok = false; std::string wf = weightFail(itemKg(it.type));
                if (!wf.empty()) {}
                else if (menuChoice == 1) { for (auto& sl : g_inv) if (!sl.type) { sl = it; ok = true; break; } }
                else if (!g_hand[1].type) { g_hand[1] = it; ok = true; }
                else if (!g_hand[0].type) { g_hand[0] = it; ok = true; }
                if (ok) { if (menuRef == 0) g_chestGem = false; else g_chestCoins = false; say(menuChoice == 1 ? "Riponi il tesoro nell'inventario." : "Tieni il tesoro in mano."); }
                else say(!wf.empty() ? wf : std::string(menuChoice == 1 ? "L'inventario è pieno (tasto I)." : "Hai entrambe le mani occupate."));
            }
            menu = 0;
        }
        menuChoice = 0;
        bool frozen = invOpen || menu || dicePhase || g_dead;

        const Uint8* k = SDL_GetKeyboardState(nullptr);
        if (!frozen) {
            if (k[SDL_SCANCODE_LEFT])  yaw -= 1.8f * dt;
            if (k[SDL_SCANCODE_RIGHT]) yaw += 1.8f * dt;
            if (k[SDL_SCANCODE_UP])    pitch += 1.4f * dt;
            if (k[SDL_SCANCODE_DOWN])  pitch -= 1.4f * dt;
        } else if (invOpen) {
            if (k[SDL_SCANCODE_LEFT])  g_figAngle -= 2.2f * dt;
            if (k[SDL_SCANCODE_RIGHT]) g_figAngle += 2.2f * dt;
        }
        pitch = std::max(-1.45f, std::min(1.45f, pitch));
        if (yaw > PI) yaw -= 2 * PI;
        if (yaw < -PI) yaw += 2 * PI;

        float fx = sinf(yaw), fz = -cosf(yaw), rx = cosf(yaw), rz = sinf(yaw);
        float mx = 0, mz = 0;
        if (!frozen) {
            if (k[SDL_SCANCODE_W]) { mx += fx; mz += fz; }
            if (k[SDL_SCANCODE_S]) { mx -= fx; mz -= fz; }
            if (k[SDL_SCANCODE_D]) { mx += rx; mz += rz; }
            if (k[SDL_SCANCODE_A]) { mx -= rx; mz -= rz; }
        }
        float ml = sqrtf(mx * mx + mz * mz);
        if (ml > 0) {
            float sp = ((k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) ? 4.2f : 2.2f) * dt / ml;
            if (!blocked(px + mx * sp, pz)) px += mx * sp;
            if (!blocked(px, pz + mz * sp)) pz += mz * sp;
        }
        float joinTarget = (!frozen && k[SDL_SCANCODE_F]) ? 1.f : 0.f;
        join += (joinTarget - join) * std::min(1.f, dt * 9.f);

        // oggetti sciolti: cadono per gravità; quello trasportato segue il giocatore
        g_hotPulse = .5f + .5f * sinf(g_time * 8.f);
        for (auto& o : g_loose) {                                         // gravità e rimbalzo (più piccolo per gli oggetti pesanti)
            float fy = floorY(o.pos.x, o.pos.z), m = looseKg(o), rest = .45f / (1.f + .5f * m);
            if (o.pos.y > fy + 1e-4f || o.vy < 0.f) {
                o.vy += 9.8f * dt; o.pos.y -= o.vy * dt;
                if (o.pos.y <= fy) { o.pos.y = fy; o.vy = o.vy > 1.4f ? -o.vy * rest : 0.f; }
            } else { o.pos.y = fy; o.vy = 0; }
            if (o.hot > 0) o.hot = std::max(0.f, o.hot - dt);
        }
        if (g_zapT > 0) g_zapT -= dt;
        if (g_zapCool > 0) g_zapCool -= dt;
        if (g_carrying) {
            g_carry.pos = {std::max(-4.6f, std::min(4.6f, px + fx * 1.0f)), .85f, std::max(-4.6f, std::min(4.6f, pz + fz * 1.0f))};
            g_carry.yaw = -yaw;
        }

        V eye{px, 1.65f, pz};
        V fwd{cosf(pitch) * sinf(yaw), sinf(pitch), -cosf(pitch) * cosf(yaw)};
        V sd{rx, 0, rz};
        V up{sd.y * fwd.z - sd.z * fwd.y, sd.z * fwd.x - sd.x * fwd.z, sd.x * fwd.y - sd.y * fwd.x};
        auto eyeToWorld = [&](float x, float y, float z) -> V {
            return {eye.x + sd.x * x + up.x * y - fwd.x * z, eye.y + sd.y * x + up.y * y - fwd.y * z, eye.z + sd.z * x + up.z * y - fwd.z * z};
        };
        V handFlame[2];
        for (int hd = 0; hd < 2; hd++) {
            float side = hd ? 1.f : -1.f;
            handFlame[hd] = eyeToWorld(side * (.30f * (1 - .7f * join) - .10f), .15f, -.79f);
        }

        // oggetti interagibili
        std::vector<HotD> hs;
        for (int i = 0; i < NHOT; i++) {
            if (i == 0 || i == 1 || i == 3 || i == 4 || i == 5 || i == 6) continue;      // ora sciolti o dinamici
            hs.push_back({std::string("Esamina: ") + HOT[i].name, HOT[i].p, HOT[i].reach, 0, i});
        }
        hs.push_back({g_swordOnWall ? "Prendi la spada" : handWith(3) >= 0 ? "Riappendi la spada ai ganci" : "Esamina: ganci della spada vuoti", HOT[5].p, HOT[5].reach, 3, 0});
        hs.push_back({g_shieldOnWall ? "Prendi lo scudo" : handWith(4) >= 0 ? "Riappendi lo scudo alla parete" : "Esamina: ganci dello scudo vuoti", HOT[6].p, HOT[6].reach, 4, 0});
        for (int i = 0; i < 4; i++) {
            std::string lb = g_wt[i].present ? (g_wt[i].lit ? "Prendi la torcia accesa" : "Prendi la torcia spenta")
                           : (handWith(2) >= 0 ? "Appendi la torcia al supporto" : "Esamina: supporto per torcia vuoto");
            hs.push_back({lb, torchAxisPos(i, .3f), 2.8f, 2, i});
        }
        for (int i = 0; i < (int)g_loose.size(); i++) {
            const Loose& o = g_loose[i]; std::string lb; V off{0, .12f, 0};
            switch (o.type) {
                case LT_BUCKET: lb = g_keyInBucket ? "Esamina: secchio di cenere" : "Prendi il secchio"; off = {0, .25f, 0}; break;
                case LT_CHEST: lb = (!g_chestOpen && hasKey()) ? "Apri lo scrigno con la chiave" : "Esamina: scrigno"; off = {0, .15f, 0}; break;
                case LT_ARMOR: case LT_GEM: case LT_COINS: case LT_BOOK: case LT_KEY: lb = std::string("Raccogli ") + looseName(o) + (o.hot > 0 ? " (ROVENTE: scotta!)" : ""); break;
                case LT_ASH: lb = "Esamina: ceneri tossiche"; break;
                case LT_SWORD: lb = "Raccogli la spada"; break;
                case LT_SHIELD: lb = "Raccogli lo scudo"; break;
                default: lb = o.lit ? "Raccogli la torcia accesa" : "Raccogli la torcia spenta"; break;
            }
            hs.push_back({lb, V{o.pos.x + off.x, o.pos.y + off.y, o.pos.z + off.z}, 2.6f, 1, i});
        }
        if (g_chestOpen)                                                          // contenuto dello scrigno aperto
            for (auto& o : g_loose) if (o.type == LT_CHEST) {
                float c = cosf(o.yaw), sn = sinf(o.yaw);
                if (g_chestGem) hs.push_back({"Gemma dello scrigno", V{o.pos.x, o.pos.y + .24f, o.pos.z}, 2.4f, 5, 0});
                if (g_chestCoins) hs.push_back({"Monete d'oro dello scrigno", V{o.pos.x + .1f * c + .05f * sn, o.pos.y + .07f, o.pos.z - .1f * sn + .05f * c}, 2.4f, 6, 0});
            }
        for (int i = 0; i < 9; i++)                                               // pezzi dell'armatura sulla rastrelliera
            if (g_rackHas[i]) hs.push_back({std::string("Prendi ") + armorArt(RACK_TYPE[i]), RACK_POS[i], 2.8f, 7, i});
        int target = -1; float best = .86f;
        if (!frozen && !g_carrying)
            for (int i = 0; i < (int)hs.size(); i++) {
                V d{hs[i].p.x - eye.x, hs[i].p.y - eye.y, hs[i].p.z - eye.z};
                float L = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
                if (L > hs[i].reach || L < .01f) continue;
                float dot = (d.x * fwd.x + d.y * fwd.y + d.z * fwd.z) / L;
                if (dot > best) { best = dot; target = i; }
            }

        if (dropPressed && !frozen && !g_carrying) {                        // Maiusc+E: lascia cadere ciò che tieni in mano
            int hd = g_hand[1].type ? 1 : g_hand[0].type ? 0 : -1;
            if (hd < 0) say("Non hai nulla in mano da lasciar cadere.", 3.f);
            else {
                Loose o = looseOfItem(g_hand[hd]); V pp{px, 0, pz};
                for (float d : {1.0f, .8f, .6f, .4f, 0.f}) { float x = px + fx * d, z = pz + fz * d; if (d == 0.f || validDrop(x, z)) { pp = {x, 0, z}; break; } }
                o.pos = {pp.x, 1.0f, pp.z}; o.yaw = -yaw; o.vy = 0;
                say(std::string("Lasci cadere ") + looseName(o) + ".", 4.f);
                g_loose.push_back(o); g_hand[hd] = Item();
            }
        }

        // tasto E: pressione breve = interagisci; tenuto premuto = trasporta l'oggetto; rilascio = cade
        bool bucketHeldUnsearched = g_keyInBucket && (g_hand[0].type == 5 || g_hand[1].type == 5);
        if (ePressed && !frozen && !g_carrying) {
            eDown = true; eTime = 0; eLong = false; pressPending = false; pressHeavy = false;
            if (target >= 0) {
                pressT = hs[target];
                if (movableHot(pressT)) pressPending = true;
                else { shortAction(pressT); pressHeavy = true; }
            } else if (bucketHeldUnsearched) { menu = 2; menuSlot = -1; }
        }
        if (eDown) {
            eTime += dt;
            if (eTime >= .35f && !eLong) {
                if (pressPending) { startCarry(pressT); pressPending = false; eLong = true; }
                else if (pressHeavy) { say("Non puoi spostarlo: è troppo pesante o fissato al muro.", 4.f); eLong = true; }
            }
        }
        if (eReleased) {
            if (g_carrying) {                                                   // lascia cadere l'oggetto, poco davanti ai piedi
                Loose o = g_carry; V pp{px, 0, pz};
                for (float d : {1.0f, .8f, .6f, .4f, 0.f}) { float x = px + fx * d, z = pz + fz * d; if (d == 0.f || validDrop(x, z)) { pp = {x, 0, z}; break; } }
                o.pos = {pp.x, .85f, pp.z}; o.yaw = -yaw; o.vy = 0;
                g_loose.push_back(o); g_carrying = false;
                say(std::string("Lasci cadere ") + looseName(o) + ".", 4.f);
            } else if (pressPending && eDown) shortAction(pressT);
            eDown = false; pressPending = false;
        }

        // accensione per contatto tra torce (in mano, appese, a terra, trasportate)
        struct Fl { V pos; bool* lit; int where; };
        std::vector<Fl> fl;
        for (int hd = 0; hd < 2; hd++) if (g_hand[hd].type == 2) fl.push_back({handFlame[hd], &g_hand[hd].lit, 0});
        for (int i = 0; i < 4; i++) if (g_wt[i].present) fl.push_back({torchAxisPos(i, .57f), &g_wt[i].lit, 1});
        for (auto& o : g_loose) if (o.type == LT_TORCH) fl.push_back({looseFlamePos(o), &o.lit, 2});
        if (g_carrying && g_carry.type == LT_TORCH) fl.push_back({looseFlamePos(g_carry), &g_carry.lit, 3});
        if (!dicePhase) fl.push_back({V{4.35f, .45f, 0.f}, &g_fireLit, 4});                // ceppi nel camino
        auto dist = [](V a, V b) { return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z)); };
        int pa = -1, pb = -1;
        if (!dicePhase)
            for (int a = 0; a < (int)fl.size() && pa < 0; a++)
                for (int b = a + 1; b < (int)fl.size(); b++) {
                    if (*fl[a].lit == *fl[b].lit) continue;
                    float thr = (fl[a].where == 4 || fl[b].where == 4) ? 1.2f : (fl[a].where == 0 && fl[b].where == 0) ? .30f : .85f;   // due mani: servono vicine (tasto F)
                    if (dist(fl[a].pos, fl[b].pos) < thr) { pa = a; pb = b; break; }
                }
        bool near = pa >= 0;
        if (near) {
            igniteT += dt;
            if (igniteT > .7f) {
                bool* un = *fl[pa].lit ? fl[pb].lit : fl[pa].lit; *un = true;
                if (un == &g_fireLit) say("Accosti la torcia ai ceppi: il fuoco divampa nel camino!");
                else if (fl[pa].where == 4 || fl[pb].where == 4) say("Accosti la torcia al fuoco del camino: si accende.");
                else say("La fiamma passa da una torcia all'altra: ora è accesa.");
                igniteT = 0;
            }
        } else igniteT = 0;

        // una torcia accesa lasciata cadere sul tavolo o accanto a una gamba lo incendia: crolla e ciò che c'è sopra cade
        if (g_tableState == 0) {
            for (auto& o : g_loose) if (o.type == LT_TORCH && o.lit) {
                bool onTable = o.pos.y > .5f && fabsf(o.pos.x) < 1.5f && fabsf(o.pos.z) < .65f;
                bool nearLeg = o.pos.y < .5f && fabsf(fabsf(o.pos.x) - 1.3f) < .6f && fabsf(fabsf(o.pos.z) - .45f) < .6f;
                if (onTable || nearLeg) { g_tableState = 1; g_tableT = 0; say("La torcia accesa incendia il legno del tavolo: le fiamme salgono lungo le gambe!", 7.f); break; }
            }
        } else if (g_tableState == 1) {
            g_tableT += dt;
            if (g_tableT > 4.5f) { g_tableState = 2; say("Il tavolo crolla in un cumulo di brace e carbone: ciò che c'era sopra cade a terra!", 7.f); }
        }

        // la rastrelliera brucia se una torcia accesa cade ai suoi piedi: l'armatura ancora appesa cade e scotta per 10 s
        if (g_rackState == 0) {
            for (auto& o : g_loose) if (o.type == LT_TORCH && o.lit && o.pos.y < .5f && o.pos.x < -3.7f && fabsf(o.pos.z) < 1.6f) {
                g_rackState = 1; g_rackT = 0; say("La torcia accesa lambisce il legno della rastrelliera: prende fuoco!", 7.f); break;
            }
        } else if (g_rackState == 1) {
            g_rackT += dt;
            if (g_rackT > 4.5f) {
                g_rackState = 2;
                for (int i = 0; i < 9; i++) if (g_rackHas[i]) {
                    Loose a{LT_ARMOR, false, RACK_POS[i], i * .7f, 0.f, RACK_TYPE[i]}; a.hot = 10.f; g_loose.push_back(a); g_rackHas[i] = false;
                }
                say("La rastrelliera crolla: l'armatura cade a terra, rovente per dieci secondi!", 8.f);
            }
        }
        updateSkeleton(dt);                                                  // le braccia cadono se il tavolo non c'è più

        // porte magiche: una fiamma avvicinata a una porta provoca una scarica (-1 vita) dopo ogni tentativo
        if (!dicePhase && !g_dead && g_zapCool <= 0.f) {
            std::vector<V> pf;
            for (int hd = 0; hd < 2; hd++) if (g_hand[hd].type == 2 && g_hand[hd].lit) pf.push_back(handFlame[hd]);
            if (g_carrying && g_carry.type == LT_TORCH && g_carry.lit) pf.push_back(looseFlamePos(g_carry));
            for (size_t fi = 0; fi < pf.size() && g_zapCool <= 0.f; fi++)
                for (int i = 0; i < 4; i++) {
                    float ddx = pf[fi].x - DOOR_C[i].x, ddz = pf[fi].z - DOOR_C[i].z;
                    float dn = ddx * DOOR_N[i].x + ddz * DOOR_N[i].z, lat = -ddx * DOOR_N[i].z + ddz * DOOR_N[i].x;
                    if (dn > -.05f && dn < .65f && fabsf(lat) < .95f && pf[fi].y > .1f && pf[fi].y < 2.9f) {
                        g_zapDoor = i; g_zapT = .7f; g_zapCool = 2.5f;
                        hurt(1, "folgorato dalla porta magica");
                        say("La porta è magica: respinge la fiamma con una scarica di luce! -1 vita.");
                        break;
                    }
                }
        }

        // il libro prende fuoco vicino a una fiamma, nel camino acceso o sul tavolo in fiamme; le ceneri sono tossiche
        if (!dicePhase) {
            for (auto& o : g_loose) if (o.type == LT_BOOK && o.burn <= 0.f) {
                V c{o.pos.x, o.pos.y + .05f, o.pos.z}; bool ig = false;
                for (auto& f : fl) if (*f.lit && dist(f.pos, c) < (f.where == 4 ? 1.2f : .85f)) { ig = true; break; }
                if (g_fireLit && o.pos.x > 3.9f && o.pos.x < 4.85f && fabsf(o.pos.z) < .9f) ig = true;
                if (g_tableState == 1 && o.pos.y > .5f) ig = true;
                if (ig) { o.burn = .01f; say("Il libro prende fuoco!", 4.f); }
            }
            for (auto& o : g_loose) if (o.type == LT_BOOK && o.burn > 0.f) {
                o.burn += dt;
                if (o.burn > 3.f) {                                           // si riduce in cenere tossica
                    o.type = LT_ASH; o.burn = 0; o.sub = 0;
                    float ddx = px - o.pos.x, ddz = pz - o.pos.z;
                    if (sqrtf(ddx * ddx + ddz * ddz) < 3.f) { startPoison(); say("Il libro si riduce in cenere tossica: i fumi ti avvelenano! (-1 vita al secondo per 10 secondi)", 8.f); }
                    else say("Il libro si riduce in cenere: i suoi residui sono tossici, tieniti lontano.", 6.f);
                }
            }
        }
        if (g_poison > 0.f && !g_dead) {                                      // veleno: -1 vita al secondo per 10 secondi
            g_poison -= dt; poisonTick += dt;
            while (poisonTick >= 1.f) { poisonTick -= 1.f; hurt(1, "intossicato dalle ceneri tossiche"); }
            if (g_poison <= 0.f) { g_poison = 0.f; poisonTick = 0.f; if (!g_dead) say("Il veleno smette di fare effetto.", 4.f); }
        }
        if (!dicePhase && g_hp > hpMax()) g_hp = hpMax();

        // minigioco dei dadi
        if (dicePhase) {
            if (shot && frame == 0 && diceShot >= 0) { startRoll(dg); dg.t = diceShot; }
            if (spacePressed) {
                if (dg.finished) { dicePhase = false; g_hp = hpMax(); say("Caratteristiche definite: la sala segreta ti aspetta. Buona esplorazione!", 8.f); }
                else if (!dg.rolling) startRoll(dg);
            }
            if (dg.rolling) {
                dg.t += dt;
                if (dg.t >= dg.TT) {
                    dg.t = dg.TT; dg.rolling = false;
                    int sum = dg.die[0].value + dg.die[1].value;
                    dg.scores[dg.skill][dg.throwNo] = sum;
                    lastDice = "Lancio " + std::to_string(dg.throwNo + 1) + " di " + STAT_NAMES[dg.skill] + ": " +
                               std::to_string(dg.die[0].value) + " + " + std::to_string(dg.die[1].value) + " = " + std::to_string(sum);
                    if (++dg.throwNo == 3) {
                        int b = std::max(dg.scores[dg.skill][0], std::max(dg.scores[dg.skill][1], dg.scores[dg.skill][2]));
                        g_stats[dg.skill] = b;
                        lastDice += "\n" + std::string(STAT_NAMES[dg.skill]) + ": il migliore dei tre lanci è " + std::to_string(b);
                        dg.skill++; dg.throwNo = 0;
                        if (dg.skill == 5) dg.finished = true;
                    }
                }
            }
        }

        if (msgT > 0) msgT -= dt;
        if (g_chestOpen) g_lid = std::min(105.f, g_lid + 130.f * dt);

        // --- rendering ---
        int w, h; SDL_GL_GetDrawableSize(win, &w, &h); lastW = w; lastH = h;
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        float nearp = .05f, farp = 60.f, fovDeg = dicePhase ? 40.f : 70.f;
        float top = nearp * tanf(fovDeg * .5f * PI / 180.f), right = top * (float)w / h;
        glFrustum(-right, right, -top, top, nearp, farp);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        if (dicePhase) lookAt(V{-.05f, 1.65f, 1.35f}, V{0, -.73f, -.95f});         // inquadratura ravvicinata sul tavolo
        else lookAt(eye, fwd);

        std::vector<V> lights;
        for (auto& f : fl) if (*f.lit) lights.push_back(f.pos);
        if (g_tableState == 1) lights.push_back(V{0, 1.1f, 0});                  // il tavolo in fiamme fa luce
        if (g_rackState == 1) lights.push_back(V{-4.6f, 1.0f, 0});               // e la rastrelliera
        glEnable(GL_FOG); glEnable(GL_LIGHTING); glEnable(GL_COLOR_MATERIAL);
        setLights(lights);
        drawWorld();
        if (dicePhase && dg.shown) for (int i = 0; i < 2; i++) drawDie(dg.die[i], dg.t / dg.TT);
        glDisable(GL_LIGHTING); glDisable(GL_FOG);

        if (!dicePhase) { drawHandItem(g_hand[0], -1, join); drawHandItem(g_hand[1], +1, join); }

        // HUD 2D
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1000, 1000);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        int mxw = 0, myw = 0, ww, wh; SDL_GetMouseState(&mxw, &myw); SDL_GetWindowSize(win, &ww, &wh);
        float mxp = mxw * (float)w / ww, myp = myw * (float)h / wh;
        InvLayout IL = invLayout(w, h);
        int hover = invOpen ? invHit(IL, mxp, myp) : -1;
        std::string hud;
        if (dicePhase) {
            tc.panelAt(scoreboard(dg), 24, 24);
            if (dg.rolling) hud = "I dadi rotolano sul tavolo...";
            else if (dg.finished) hud = (lastDice.empty() ? "" : lastDice + "\n") + "Tutte le caratteristiche sono definite. Premi SPAZIO per entrare nella sala.";
            else hud = (lastDice.empty() ? "" : lastDice + "\n") + STAT_NAMES[dg.skill] + " — lancio " + std::to_string(dg.throwNo + 1) + " di 3 · premi SPAZIO per lanciare i dadi";
        } else {
            if (!invOpen) {
                if (target >= 0) col(1.f, .85f, .2f); else col(.8f, .85f, .95f);
                float c = target >= 0 ? 10.f : 6.f; float cx = w / 2.f, cy = h / 2.f;
                line(cx - c, cy, 0, cx + c, cy, 0); line(cx, cy - c, 0, cx, cy + c, 0);
            } else drawInventory(tc, w, h, hover);

            if (menu == 1) hud = "Un secchio di legno pieno di cenere spenta.\n1 – Fruga nella cenere\n2 – Tieni nell'inventario\nEsc – Annulla";
            else if (menu == 2) hud = "Il secchio non è ancora stato frugato.\n1 – Fruga nella cenere (la chiave va con te)\n2 – " +
                                      std::string(menuSlot < 0 ? "Lascia stare" : menuSlot >= 9 ? "Riponilo nell'inventario" : "Prendilo in mano") + "\nEsc – Annulla";
            else if (menu == 3) hud = "Hai la chiave di bronzo. Cosa vuoi fare con lo scrigno?\n1 – Apri il lucchetto\n2 – Prova a sollevarlo\nEsc – Annulla";
            else if (menu == 4) hud = std::string(menuRef == 0 ? "Una gemma azzurra" : "Un mucchio di monete d'oro") + ". Cosa vuoi fare?\n1 – Riponi nell'inventario\n2 – Tieni in mano\nEsc – Annulla";
            else if (invOpen) {
                if (hover >= 0) {
                    if (hover >= 11) {
                        int ix = hover - 11;
                        hud = g_equip[ix] ? std::string(SLOT_NAME[ix]) + ": indossato — clic: toglilo (in mano o nell'inventario)"
                                          : std::string(SLOT_NAME[ix]) + " — vuoto: tieni in mano il pezzo e clicca qui per indossarlo";
                    } else if (hover >= 9) {
                        const Item& it = g_hand[hover - 9]; const char* nm = hover == 9 ? "sinistra" : "destra";
                        hud = it.type ? std::string("Mano ") + nm + ": " + itemName(it) + (it.type == 5 && g_keyInBucket ? " — clic: scegli cosa fare" : " — clic per riporla nell'inventario")
                                      : std::string("Mano ") + nm + " vuota";
                    } else {
                        const Item& it = g_inv[hover];
                        if (it.type) hud = itemName(it) + (it.type == 5 && g_keyInBucket ? " — clic: scegli (frugare o prenderlo in mano)" : " — clic: mano destra · clic destro: mano sinistra") + (isArmor(it.type) ? " · Ctrl+clic: indossa" : "");
                        else hud = (g_hand[0].type || g_hand[1].type) ? "Slot vuoto — clic: riponi dalla mano destra · clic destro: dalla sinistra" : "Slot vuoto";
                    }
                } else if (figHit(IL, mxp, myp)) hud = "Trascina col mouse (o frecce sinistra/destra) per ruotare la figura a 360°";
                else if (msgT > 0) hud = msg;
                else hud = "Clic: mano destra · clic destro o Maiusc+1–9: mano sinistra · I o Esc per chiudere";
            }
            else if (msgT > 0) hud = msg;
            else if (g_carrying) hud = std::string("Trasporti ") + looseName(g_carry) + " — rilascia E per lasciarlo cadere";
            else if (near && igniteT > 0) hud = "Le fiamme si toccano...";
            else if (target >= 0) hud = "[E] " + hs[target].label + (movableHot(hs[target]) ? " · tieni premuto per trasportare" : "");
            else if (bucketHeldUnsearched) hud = "[E] Fruga nella cenere del secchio che tieni in mano";
            else if (g_hand[0].type == 2 && g_hand[1].type == 2 && g_hand[0].lit != g_hand[1].lit) hud = "Tieni premuto [F] per accostare le due torce";
            else if (g_hand[0].type || g_hand[1].type) hud = "[Maiusc+E] Lascia cadere ciò che tieni in mano";
        }
        if (!dicePhase) {                                                     // vita, veleno (clessidra) e morte
            tc.panelAt("Vita: " + std::to_string(g_hp) + " / " + std::to_string(hpMax()) + (g_poison > 0 ? "  ·  intossicato" : ""), 24, 24);
            if (g_poison > 0) drawHourglass(tc, w - 70.f, h - 130.f, 100.f, g_poison / 10.f);
            if (g_dead) { fillRect(0, 0, (float)w, (float)h, .35f, 0, 0, .55f); hud = "Sei morto: " + g_deathCause + ".\nPremi R per ricominciare · Esc per uscire"; }
        }
        tc.panel(hud, w, h);
        SDL_GL_SwapWindow(win);

        if (shot && ++frame >= 3) { savePPM(shot, w, h); printf("screenshot: %s\n", shot); break; }
    }
    tc.shutdown();
    SDL_GL_DeleteContext(ctx); SDL_DestroyWindow(win); SDL_Quit();
    if (restart) execv(argv[0], argv);                                   // R dopo la morte: riavvia la partita
    return 0;
}
