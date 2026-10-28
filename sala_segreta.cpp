// ============================================================================
//  LA SALA SEGRETA — dungeon 3D in wireframe, navigabile in prima persona
//  C++17 + SDL2 + OpenGL (pipeline fissa, nessuna libreria extra: niente GLU/GLEW)
//
//  Compilazione (Linux): dipendenze e procedura complete in INSTALL.md
//    Arch:  sudo pacman -S --needed base-devel pkgconf sdl2 sdl2_ttf libglvnd fontconfig urw-base35-fonts
//    make -j$(nproc)            (oppure: bash tools/build.sh   |   cmake -S . -B build && cmake --build build)
//    ./build/sala_segreta
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
//  Opzioni di test:  ./sala_segreta --shot out.ppm [x z yaw_gradi pitch_gradi] [--deterministic] [--presse] [--dead] [--demo] [--inv]
//                    [--fig gradi] [--skipdice] [--dicetime s] [--carry] [--menu] [--fire] [--burn] [--burnt] [--open] [--worn]
//                    [--rackburn] [--rackburnt] [--poison s] [--hp n] [--zap]
//  --shot implica --deterministic: passo fisso 1/60 s e seme fisso dei dadi, quindi screenshot riproducibili (vedi tools/).
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

#include "core/config.h"
#include "core/types.h"
#include "gfx/primitives.h"
#include "state.h"
#include "render/render.h"
#include "items.h"
#include "ui/text.h"
#include "ui/draw2d.h"
#include "ui/figure.h"
#include "ui/hourglass.h"
#include "ui/inventory_ui.h"
#include "skeleton_physics.h"


// Stato di gioco usato solo dalla logica (quello condiviso con il disegno è in state.h / state.cpp)
static float g_zapCool = 0;                    // pausa tra due scariche della porta magica
static float g_poison = 0;                     // secondi di intossicazione rimasti
static bool g_dead = false; static std::string g_deathCause;

// ----------------------------------------------------------------------------
//  Oggetti e inventario (stato condiviso con il disegno: state.h)
// ----------------------------------------------------------------------------

static float floorY(float x, float z) {                          // piano del tavolo, focolare o pavimento
    if (x > 3.9f && x < 4.85f && fabsf(z) < .9f) return .1f;
    return (tableState() != 2 && fabsf(x) < 1.5f && fabsf(z) < .65f) ? .9f : 0.f;
}
static V looseFlamePos(const Entity& o) {                        // fiamma di una torcia a terra (testa verso +x locale)
    float c = cosf(o.yaw), sn = sinf(o.yaw);
    return {o.pos.x + .44f * c, o.pos.y + .18f, o.pos.z - .44f * sn};
}

static bool hasKey() {
    for (auto& h : g_hand) if (h.def == D_KEY) return true;
    for (auto& s : g_inv) if (s.def == D_KEY) return true;
    return false;
}
static int handWith(int type) { if (g_hand[1].def == type) return 1; if (g_hand[0].def == type) return 0; return -1; }
// mette l'oggetto in mano (prima la destra) e/o nell'inventario; false se non c'è posto
static std::string g_giveFail;
static std::string weightFail(float kg) {                          // "" se si può portare, altrimenti il motivo
    if (totalKg() + kg <= MAX_KG + 1e-3f) return "";
    return "Troppo pesante: porti già " + fmtKg(totalKg()) + " kg su " + fmtKg(MAX_KG) + " e questo ne pesa " + fmtKg(kg) + ".";
}
static std::string giveFailMsg() { return g_giveFail.empty() ? "Hai mani e inventario pieni (tasto I)." : g_giveFail; }
static bool giveItem(Entity it, bool preferHand) {
    g_giveFail = weightFail(itemKg(it.def));
    if (!g_giveFail.empty()) return false;
    auto toHand = [&]() { if (!g_hand[1].def) { g_hand[1] = it; return true; } if (!g_hand[0].def) { g_hand[0] = it; return true; } return false; };
    auto toInv = [&]() { for (auto& s : g_inv) if (!s.def) { s = it; return true; } return false; };
    bool ok = preferHand ? (toHand() || toInv()) : (toInv() || toHand());
    if (!ok) g_giveFail = "Hai mani e inventario pieni (tasto I).";
    return ok;
}
static const V RACK_POS[9] = {{-4.75f, 1.2f, 0}, {-4.75f, 1.25f, -.58f}, {-4.75f, 1.25f, .58f}, {-4.75f, .9f, -.70f}, {-4.75f, .9f, .70f},
                              {-4.75f, .88f, -.22f}, {-4.75f, .88f, .22f}, {-4.82f, .35f, -.65f}, {-4.82f, .35f, .65f}};
// ---- pesi (kg) ----
static V wallPoint(const Wall& w, float u, float lx, float ly, float lz) {
    return {w.ox + w.ux * (u + lx) + w.nx * lz, ly, w.oz + w.uz * (u + lx) + w.nz * lz};
}
// punto lungo l'asse della torcia appesa i (t = distanza dal punto di appoggio)
static V torchAxisPos(int i, float t) {
    float a = TORCH_TILT * PI / 180.f;
    return wallPoint(W[i], TORCH_U[i], 0, TORCH_Y + cosf(a) * t, TORCH_Z + sinf(a) * t);
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
            Entity it{D_TORCH, g_wt[i].lit};
            if (!giveItem(it, true)) return giveFailMsg();
            g_wt[i].present = false;
            return it.lit ? "Stacchi la torcia accesa dal supporto." : "Stacchi la torcia spenta dal supporto.";
        }
        int h = handWith(D_TORCH);
        if (h >= 0) { g_wt[i] = {true, g_hand[h].lit}; g_hand[h] = Entity(); return "Appendi la torcia al supporto di ferro."; }
        return "Un supporto di ferro per torce, vuoto.";
    }
    if (id == 5 || id == 6) {                                   // spada e scudo: prelevabili e riappendibili
        bool sw = id == 5; Entity& hooks = fixture(sw ? D_SWORD_HOOKS : D_SHIELD_HOOKS); int type = sw ? D_SWORD : D_SHIELD;
        if (!hooks.contents.empty()) {
            if (!giveItem(slotOf(hooks.contents[0]), true)) return giveFailMsg();
            unhang(hooks, 0);
            return sw ? "Sollevi la spada dai ganci: pesante, ma ben bilanciata." : "Stacchi lo scudo dalla parete: il legno è ancora solido.";
        }
        int h = handWith(type);
        if (h >= 0) { hang(hooks, g_hand[h], 0); g_hand[h] = Entity(); return sw ? "Riponi la spada sui ganci sopra il camino." : "Riappendi lo scudo alla parete."; }
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
static bool blocked(float x, float z) {
    if (x < -LIM || x > LIM || z < -LIM || z > LIM) return true;
    for (int i = 0; i < (int)(sizeof(SOLID) / sizeof(SOLID[0])); i++) {
        if (i == 0 && tableState() == 2) continue;                       // il tavolo è bruciato: non blocca più
        if (i == 3 && rackState() == 2) continue;                        // idem la rastrelliera
        const AABB& b = SOLID[i];
        if (x > b.x0 - PR && x < b.x1 + PR && z > b.z0 - PR && z < b.z1 + PR) return true;
    }
    return false;
}


// ----------------------------------------------------------------------------
//  Riquadro di testo in basso (SDL_ttf -> texture OpenGL)
// ----------------------------------------------------------------------------


// ---- figura umana in wireframe (per l'inventario) ----


// ---- inventario ----
static void slotAction(int i, int hand) {                       // hand: 0 = sinistra, 1 = destra
    if (i == 9 || i == 10) {                                    // clic su una mano: riponi nell'inventario
        Entity& hh = g_hand[i - 9];
        if (hh.def) for (auto& s : g_inv) if (!s.def) { s = hh; hh = Entity(); break; }
    } else if (i >= 0 && i < 9) std::swap(g_hand[hand], g_inv[i]);   // prende, ripone o scambia
}
static bool autoEquip(Entity& it) {              // indossa il pezzo nel primo slot libero del suo tipo
    if (!isArmor(it.def)) return false;
    for (int i = 0; i < 10; i++) if (SLOT_TYPE[i] == it.def && !g_equip[i]) { g_equip[i] = true; it = Entity(); return true; }
    return false;
}
static std::string equipClick(int idx, int hand) {   // clic su uno slot di equipaggiamento
    if (g_equip[idx]) {                                     // toglie il pezzo: in mano (se libera) o nell'inventario
        Entity it{SLOT_TYPE[idx], false};
        if (!g_hand[hand].def) g_hand[hand] = it;
        else {
            bool ok = false;
            for (auto& sl : g_inv) if (!sl.def) { sl = it; ok = true; break; }
            if (!ok) return "Mani e inventario pieni: non puoi togliere il pezzo.";
        }
        g_equip[idx] = false;
        return itemName(it) + " tolto.";
    }
    if (g_hand[hand].def == SLOT_TYPE[idx]) { Entity it = g_hand[hand]; g_equip[idx] = true; g_hand[hand] = Entity(); return itemName(it) + " indossato."; }
    return "Tieni in mano il pezzo giusto (clic: mano destra · clic destro: sinistra) e poi clicca lo slot per indossarlo.";
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

// ----------------------------------------------------------------------------
//  Programma principale
// ----------------------------------------------------------------------------

static void savePPM(const char* path, int w, int h) {
    std::vector<unsigned char> px((size_t)w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    FILE* f = fopen(path, "wb"); if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; y--) fwrite(&px[(size_t)y * w * 3], 1, (size_t)w * 3, f);
    fclose(f);
}


struct HotD { std::string label; V p; float reach; int kind, idx, idx2 = 0; };   // kind: 0 statico, 1 sciolto, 2 torcia a muro, 3 spada, 4 scudo, 5 contenuto di un contenitore (idx = contenitore in g_loose, idx2 = posizione nel contenuto), 7 pezzi sulla rastrelliera
static bool movableHot(const HotD& h) {      // kind 5: contenuto di un contenitore; 7: pezzi sulla rastrelliera
    return (h.kind == 1 && g_loose[h.idx].def != D_ASH) || h.kind == 5 || (h.kind == 7 && rackHas(h.idx)) ||
           (h.kind == 2 && g_wt[h.idx].present) || (h.kind == 3 && swordOnWall()) || (h.kind == 4 && shieldOnWall());
}
static const V DOOR_C[4] = {{0, 0, -5}, {0, 0, 5}, {5, 0, 3.4f}, {-5, 0, -3.4f}};       // centro delle porte sul muro (x,z)
static const V DOOR_N[4] = {{0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0}};                // normali verso l'interno della sala
static bool validDrop(float x, float z) {                                  // dove si può lasciar cadere un oggetto
    if (fabsf(x) > 4.55f || fabsf(z) > 4.55f) return false;
    if (x > 3.9f && x < 4.85f && fabsf(z) < .9f) return true;              // dentro il focolare (si può buttare nel fuoco)
    if (x > 3.8f && fabsf(z) < 1.5f) return false;                         // resto del camino
    if (rackState() != 2 && x < -4.3f && fabsf(z) < 1.35f) return false;   // rastrelliera
    if (x > 1.7f && x < 2.55f && fabsf(z) < .45f) return false;            // sedia e scheletro
    return true;
}

int main(int argc, char** argv) {
    const char* shot = nullptr; bool deterministic = false, pressE = false, deadTest = false; bool demo = false, invOpen = false, dragging = false, skipDice = false, carryDemo = false, menuDemo = false;
    float px = -3.6f, pz = 3.6f, yaw = PI / 4, pitch = -.12f, diceShot = -1.f; bool rackBurnt = false, openChestOpt = false; int hpArg = 0;
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
        else if (!strcmp(argv[i], "--burn")) { fixture(D_TABLE).fireState = 1; fixture(D_TABLE).fireT = 2.f; }
        else if (!strcmp(argv[i], "--burnt")) fixture(D_TABLE).fireState = 2;
        else if (!strcmp(argv[i], "--open")) openChestOpt = true;
        else if (!strcmp(argv[i], "--worn")) { for (bool& b : g_equip) b = true; fixture(D_RACK).contents.clear(); }
        else if (!strcmp(argv[i], "--rackburn")) { fixture(D_RACK).fireState = 1; fixture(D_RACK).fireT = 2.5f; }
        else if (!strcmp(argv[i], "--rackburnt")) rackBurnt = true;
        else if (!strcmp(argv[i], "--poison") && i + 1 < argc) g_poison = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--hp") && i + 1 < argc) hpArg = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--zap")) { g_zapT = .7f; g_zapDoor = 2; }
        else if (!strcmp(argv[i], "--deterministic")) deterministic = true;
        else if (!strcmp(argv[i], "--presse")) pressE = true;
        else if (!strcmp(argv[i], "--dead")) deadTest = true;
        else if (!strcmp(argv[i], "--dicetime") && i + 1 < argc) diceShot = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--fig") && i + 1 < argc) g_figAngle = (float)atof(argv[++i]) * PI / 180;
    }
    if (openChestOpt) for (Entity& o : g_loose) if (o.def == D_CHEST) o.open = true;      // --open: lo scrigno parte aperto
    bool dicePhase = (!shot || diceShot >= 0) && !demo && !skipDice;     // all'avvio: minigioco dei dadi
    if (!dicePhase) { g_stats[0] = 9; g_stats[1] = 8; g_stats[2] = 7; g_stats[3] = 10; g_stats[4] = 11; }
    if (demo) {
        g_hand[1] = Entity{D_TORCH, true}; g_hand[0] = Entity{D_SHIELD, false};
        g_inv[0] = Entity{D_KEY, false}; g_inv[1] = Entity{D_TORCH, false}; g_inv[2] = Entity{D_SWORD, false}; g_inv[4] = Entity{D_TORCH, true}; g_inv[5] = Entity{D_BUCKET, false};
        fixture(D_SWORD_HOOKS).contents.clear(); fixture(D_SHIELD_HOOKS).contents.clear(); g_loose.erase(g_loose.begin());   // (il secchio è in inventario, già frugato)
    }
    if (rackBurnt) {                                  // test: rastrelliera già crollata, pezzi a terra e roventi
        fixture(D_RACK).fireState = 2;
        V sp[9]; for (int i = 0; i < 9; i++) sp[i] = V{RACK_POS[i].x + .5f, 0.f, RACK_POS[i].z};
        spillSupport(fixture(D_RACK), g_loose, sp, 10.f);
    }
    if (tableState() == 2) for (int i = 0; i < 700; i++) updateSkeleton(.016f);   // test: braccia già assestate
    if (!dicePhase) { g_hp = hpMax(); if (hpArg > 0) g_hp = std::min(hpArg, hpMax()); }
    if (deadTest) { g_hp = 0; g_dead = true; g_deathCause = "intossicato dalle ceneri tossiche"; }
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
    if (deterministic || shot) dg.rng.seed(20261024u);                     // seme fisso: stessi dadi a ogni prova
    // menu a scelta multipla: 0 nessuno; 1 secchio a terra non frugato; 2 secchio in slot/mano non frugato
    int menu = menuDemo ? 1 : 0, menuRef = 0, menuSub = 0, menuSlot = -1, menuHand = 1, menuChoice = 0;
    bool eDown = false, eLong = false, pressPending = false, pressHeavy = false; float eTime = 0; HotD pressT{};

    // ---- azioni (lambda) ----
    auto say = [&](const std::string& m, float t = 6.f) { msg = m; msgT = t; if (!tc.font) { printf("%s\n", m.c_str()); fflush(stdout); } };
    auto menuContent = [&]() -> const Entity* {                                    // l'oggetto indicato dal menu 4 (dentro un contenitore)
        return (menuRef >= 0 && menuRef < (int)g_loose.size() && menuSub >= 0 && menuSub < (int)g_loose[menuRef].contents.size()) ? &g_loose[menuRef].contents[menuSub] : nullptr;
    };
    auto hurt = [&](int n, const char* cause) { if (g_dead) return; g_hp -= n; if (g_hp <= 0) { g_hp = 0; g_dead = true; g_deathCause = cause; } };
    auto startPoison = [&]() { if (g_poison <= 0) poisonTick = 0; g_poison = 10.f; };
    auto searchBucket = [&](Entity& b) {                                           // frugare un secchio: la chiave nel suo contenuto passa a chi lo fruga
        int ki = contentIndex(b, D_KEY);
        if (ki < 0) { say("Frughi nella cenere, ma non c'è più nulla."); return; }
        Entity key = b.contents[ki];
        if (giveItem(slotOf(key), false)) { b.contents.erase(b.contents.begin() + ki); say("Frughi nella cenere fredda... e senti del metallo: una CHIAVE DI BRONZO! La metti con te."); }
        else say("Senti qualcosa sotto la cenere, ma non riesci a prenderlo. " + giveFailMsg());
    };
    auto shortLoose = [&](int k) {
        Entity o = g_loose[k];
        if (o.hot > 0) { hurt(1, "ustionato dall'armatura rovente"); say("Scotta! Il metallo è ancora rovente: -1 vita."); return; }
        switch (o.def) {
            case D_BUCKET:
                if (holds(o, D_KEY)) { menu = 1; menuRef = k; }
                else if (giveItem(slotOf(o), true)) { g_loose.erase(g_loose.begin() + k); say("Raccogli il secchio, ormai vuoto di segreti."); }
                else say(giveFailMsg());
                break;
            case D_CHEST:
                if (o.open) say(!o.contents.empty() ? "Lo scrigno è aperto: dentro luccica ancora qualcosa. Puntalo e premi E." : "Lo scrigno è aperto e vuoto.");
                else if (hasKey()) { menu = 3; menuRef = k; }                                       // scelta multipla: hai la chiave
                else say("Lo scrigno è chiuso da un grosso lucchetto. Serve una chiave.");
                break;
            case D_ASH: startPoison(); say("Le ceneri tossiche si alzano in una nube: ti senti male! Veleno per 10 secondi."); break;
            default: {
                Entity it = slotOf(o);
                if (giveItem(it, true)) { g_loose.erase(g_loose.begin() + k); say(std::string("Raccogli ") + defArticle(o.def) + "."); }
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
        else if (t.kind == 5) { menu = 4; menuRef = t.idx; menuSub = t.idx2; }      // contenuto dello scrigno: inventario o mano
        else if (t.kind == 7 && rackState() == 1) { hurt(1, "ustionato dalla rastrelliera in fiamme"); say("Le fiamme della rastrelliera ti scottano: -1 vita."); }
        else if (t.kind == 7) {
            Entity& rack = fixture(D_RACK); int ri = hungIndex(rack, t.idx);
            Entity it = ri >= 0 ? slotOf(rack.contents[ri]) : Entity{};
            if (ri >= 0 && giveItem(it, true)) { unhang(rack, ri); say(std::string("Prendi ") + defArticle(it.def) + " dalla rastrelliera."); }
            else say(giveFailMsg());
        }
    };
    auto startCarry = [&](const HotD& t) {
        Entity c{};
        if (t.kind == 1) c = g_loose[t.idx];
        else if (t.kind == 2) c = Entity{D_TORCH, g_wt[t.idx].lit, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 3) c = Entity{D_SWORD, false, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 4) c = Entity{D_SHIELD, false, V{0, 0, 0}, 0.f, 0.f};
        else if (t.kind == 5) c = g_loose[t.idx].contents[t.idx2];
        else if (t.kind == 7) { int ri = hungIndex(fixture(D_RACK), t.idx); if (ri < 0) return; c = slotOf(fixture(D_RACK).contents[ri]); }
        else return;
        if (c.def == D_ASH) { startPoison(); say("Le ceneri tossiche si alzano in una nube: ti senti male! Veleno per 10 secondi."); return; }
        if (c.hot > 0 || (t.kind == 7 && rackState() == 1)) { hurt(1, "ustionato"); say("Scotta! È rovente: -1 vita."); return; }
        std::string wf = weightFail(itemKg(c.def));
        if (!wf.empty()) { say(wf + " Non riesci a sollevarlo."); return; }
        if (t.kind == 1) g_loose.erase(g_loose.begin() + t.idx);                    // lo toglie dalla sua posizione
        else if (t.kind == 2) g_wt[t.idx].present = false;
        else if (t.kind == 3) unhang(fixture(D_SWORD_HOOKS), 0);
        else if (t.kind == 4) unhang(fixture(D_SHIELD_HOOKS), 0);
        else if (t.kind == 5) g_loose[t.idx].contents.erase(g_loose[t.idx].contents.begin() + t.idx2);
        else if (t.kind == 7) unhang(fixture(D_RACK), hungIndex(fixture(D_RACK), t.idx));
        g_carry = c; g_carrying = true;
        say(std::string("Stai trasportando ") + defArticle(g_carry.def) + " (" + fmtKg(itemKg(g_carry.def)) + " kg). Rilascia E per lasciarlo cadere.", 8.f);
    };
    auto invActivate = [&](int slot, int hand, bool ctrl) {
        Entity& it = slot >= 9 ? g_hand[slot - 9] : g_inv[slot];
        if (ctrl && isArmor(it.def)) { say(autoEquip(it) ? "Pezzo indossato." : "Hai già indossato tutti i pezzi di quel tipo.", 4.f); }
        else if (it.def == D_BUCKET && holds(it, D_KEY)) { menu = 2; menuSlot = slot; menuHand = hand; }
        else slotAction(slot, hand);
    };

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (deterministic || shot) ? 1.f / 60.f                       // passo fisso: animazioni riproducibili
                                           : std::min(.05f, (float)(now - last) / SDL_GetPerformanceFrequency());
        last = now;
        g_time += dt;
        bool ePressed = false, eReleased = false, spacePressed = false, dropPressed = false;
        if (shot && pressE && frame == 1 && !dicePhase) ePressed = true;       // --shot --presse: preme E sull'oggetto puntato

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
                if (menuChoice == 1) searchBucket(g_loose[menuRef]);
                else {
                    bool ok = false; std::string wf = weightFail(itemKg(D_BUCKET));
                    if (wf.empty()) for (auto& sl : g_inv) if (!sl.def) { sl = slotOf(g_loose[menuRef]); g_loose.erase(g_loose.begin() + menuRef); ok = true; break; }
                    say(ok ? "Tieni il secchio nell'inventario: potrai frugarlo più tardi." : !wf.empty() ? wf : "L'inventario è pieno (tasto I).");
                }
            } else if (menu == 2) {
                if (menuChoice == 1) { if (menuSlot >= 0) searchBucket(menuSlot >= 9 ? g_hand[menuSlot - 9] : g_inv[menuSlot]); }
                else if (menuSlot >= 0) slotAction(menuSlot, menuHand);
            } else if (menu == 3) {                                                    // scrigno chiuso, con la chiave
                if (menuChoice == 1) { g_loose[menuRef].open = true; say("La chiave di bronzo gira nel lucchetto: clic! Il coperchio si solleva: dentro una gemma e delle monete d'oro."); }
                else say("Lo scrigno è pesante: tieni premuto E per trasportarlo. Non entra nell'inventario.");
            } else if (menu == 4) {                                                    // contenuto: inventario o mano
                const Entity* cp = menuContent();
                Entity it = cp ? slotOf(*cp) : Entity{}; bool ok = false; std::string wf = it.def ? weightFail(itemKg(it.def)) : "";
                if (!wf.empty()) {}
                else if (menuChoice == 1) { for (auto& sl : g_inv) if (!sl.def) { sl = it; ok = true; break; } }
                else if (!g_hand[1].def) { g_hand[1] = it; ok = true; }
                else if (!g_hand[0].def) { g_hand[0] = it; ok = true; }
                if (ok) { g_loose[menuRef].contents.erase(g_loose[menuRef].contents.begin() + menuSub); say(menuChoice == 1 ? "Riponi il tesoro nell'inventario." : "Tieni il tesoro in mano."); }
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
            float fy = floorY(o.pos.x, o.pos.z), m = itemKg(o.def), rest = .45f / (1.f + .5f * m);
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
        hs.push_back({swordOnWall() ? "Prendi la spada" : handWith(D_SWORD) >= 0 ? "Riappendi la spada ai ganci" : "Esamina: ganci della spada vuoti", HOT[5].p, HOT[5].reach, 3, 0});
        hs.push_back({shieldOnWall() ? "Prendi lo scudo" : handWith(D_SHIELD) >= 0 ? "Riappendi lo scudo alla parete" : "Esamina: ganci dello scudo vuoti", HOT[6].p, HOT[6].reach, 4, 0});
        for (int i = 0; i < 4; i++) {
            std::string lb = g_wt[i].present ? (g_wt[i].lit ? "Prendi la torcia accesa" : "Prendi la torcia spenta")
                           : (handWith(D_TORCH) >= 0 ? "Appendi la torcia al supporto" : "Esamina: supporto per torcia vuoto");
            hs.push_back({lb, torchAxisPos(i, .3f), 2.8f, 2, i});
        }
        for (int i = 0; i < (int)g_loose.size(); i++) {
            const Entity& o = g_loose[i]; std::string lb; V off{0, .12f, 0};
            switch (o.def) {
                case D_BUCKET: lb = holds(o, D_KEY) ? "Esamina: secchio di cenere" : "Prendi il secchio"; off = {0, .25f, 0}; break;
                case D_CHEST: lb = (!o.open && hasKey()) ? "Apri lo scrigno con la chiave" : "Esamina: scrigno"; off = {0, .15f, 0}; break;
                case D_HELM: case D_CUIRASS: case D_PAULDRON: case D_GREAVE: case D_VAMBRACE: case D_BOOT: case D_GEM: case D_COINS: case D_BOOK: case D_KEY: lb = std::string("Raccogli ") + defArticle(o.def) + (o.hot > 0 ? " (ROVENTE: scotta!)" : ""); break;
                case D_ASH: lb = "Esamina: ceneri tossiche"; break;
                case D_SWORD: lb = "Raccogli la spada"; break;
                case D_SHIELD: lb = "Raccogli lo scudo"; break;
                default: lb = o.lit ? "Raccogli la torcia accesa" : "Raccogli la torcia spenta"; break;
            }
            hs.push_back({lb, V{o.pos.x + off.x, o.pos.y + off.y, o.pos.z + off.z}, 2.6f, 1, i});
        }
        for (int ci = 0; ci < (int)g_loose.size(); ci++) {                        // contenuto di uno scrigno aperto
            const Entity& o = g_loose[ci];
            if (o.def != D_CHEST || !o.open) continue;
            float c = cosf(o.yaw), sn = sinf(o.yaw);
            for (int j = 0; j < (int)o.contents.size(); j++) {
                int d = o.contents[j].def;
                if (d == D_GEM) hs.push_back({"Gemma dello scrigno", V{o.pos.x, o.pos.y + .24f, o.pos.z}, 2.4f, 5, ci, j});
                else if (d == D_COINS) hs.push_back({"Monete d'oro dello scrigno", V{o.pos.x + .1f * c + .05f * sn, o.pos.y + .07f, o.pos.z - .1f * sn + .05f * c}, 2.4f, 5, ci, j});
            }
        }
        for (const Entity& p : fixture(D_RACK).contents)                               // pezzi dell'armatura sulla rastrelliera
            hs.push_back({std::string("Prendi ") + defArticle(p.def), RACK_POS[p.slot], 2.8f, 7, p.slot});
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
            int hd = g_hand[1].def ? 1 : g_hand[0].def ? 0 : -1;
            if (hd < 0) say("Non hai nulla in mano da lasciar cadere.", 3.f);
            else {
                Entity o = g_hand[hd]; V pp{px, 0, pz};
                for (float d : {1.0f, .8f, .6f, .4f, 0.f}) { float x = px + fx * d, z = pz + fz * d; if (d == 0.f || validDrop(x, z)) { pp = {x, 0, z}; break; } }
                o.pos = {pp.x, 1.0f, pp.z}; o.yaw = -yaw; o.vy = 0;
                say(std::string("Lasci cadere ") + defArticle(o.def) + ".", 4.f);
                g_loose.push_back(o); g_hand[hd] = Entity();
            }
        }

        // tasto E: pressione breve = interagisci; tenuto premuto = trasporta l'oggetto; rilascio = cade
        bool bucketHeldUnsearched = (g_hand[0].def == D_BUCKET && holds(g_hand[0], D_KEY)) || (g_hand[1].def == D_BUCKET && holds(g_hand[1], D_KEY));
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
                Entity o = g_carry; V pp{px, 0, pz};
                for (float d : {1.0f, .8f, .6f, .4f, 0.f}) { float x = px + fx * d, z = pz + fz * d; if (d == 0.f || validDrop(x, z)) { pp = {x, 0, z}; break; } }
                o.pos = {pp.x, .85f, pp.z}; o.yaw = -yaw; o.vy = 0;
                g_loose.push_back(o); g_carrying = false;
                say(std::string("Lasci cadere ") + defArticle(o.def) + ".", 4.f);
            } else if (pressPending && eDown) shortAction(pressT);
            eDown = false; pressPending = false;
        }

        // accensione per contatto tra torce (in mano, appese, a terra, trasportate)
        struct Fl { V pos; bool* lit; int where; };
        std::vector<Fl> fl;
        for (int hd = 0; hd < 2; hd++) if (g_hand[hd].def == D_TORCH) fl.push_back({handFlame[hd], &g_hand[hd].lit, 0});
        for (int i = 0; i < 4; i++) if (g_wt[i].present) fl.push_back({torchAxisPos(i, .57f), &g_wt[i].lit, 1});
        for (auto& o : g_loose) if (o.def == D_TORCH) fl.push_back({looseFlamePos(o), &o.lit, 2});
        if (g_carrying && g_carry.def == D_TORCH) fl.push_back({looseFlamePos(g_carry), &g_carry.lit, 3});
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
        Entity& table = fixture(D_TABLE);
        if (table.fireState == 0) {
            for (auto& o : g_loose) if (o.def == D_TORCH && o.lit) {
                bool onTable = o.pos.y > .5f && fabsf(o.pos.x) < 1.5f && fabsf(o.pos.z) < .65f;
                bool nearLeg = o.pos.y < .5f && fabsf(fabsf(o.pos.x) - 1.3f) < .6f && fabsf(fabsf(o.pos.z) - .45f) < .6f;
                if (onTable || nearLeg) { table.fireState = 1; table.fireT = 0; say("La torcia accesa incendia il legno del tavolo: le fiamme salgono lungo le gambe!", 7.f); break; }
            }
        } else if (table.fireState == 1) {
            table.fireT += dt;
            if (table.fireT > 4.5f) { table.fireState = 2; say("Il tavolo crolla in un cumulo di brace e carbone: ciò che c'era sopra cade a terra!", 7.f); }
        }

        // la rastrelliera brucia se una torcia accesa cade ai suoi piedi: l'armatura ancora appesa cade e scotta per 10 s
        Entity& rackE = fixture(D_RACK);
        if (rackE.fireState == 0) {
            for (auto& o : g_loose) if (o.def == D_TORCH && o.lit && o.pos.y < .5f && o.pos.x < -3.7f && fabsf(o.pos.z) < 1.6f) {
                rackE.fireState = 1; rackE.fireT = 0; say("La torcia accesa lambisce il legno della rastrelliera: prende fuoco!", 7.f); break;
            }
        } else if (rackE.fireState == 1) {
            rackE.fireT += dt;
            if (rackE.fireT > 4.5f) {
                rackE.fireState = 2;
                spillSupport(rackE, g_loose, RACK_POS, 10.f);                  // l'armatura ancora appesa cade e scotta per 10 s
                say("La rastrelliera crolla: l'armatura cade a terra, rovente per dieci secondi!", 8.f);
            }
        }
        updateSkeleton(dt);                                                  // le braccia cadono se il tavolo non c'è più

        // porte magiche: una fiamma avvicinata a una porta provoca una scarica (-1 vita) dopo ogni tentativo
        if (!dicePhase && !g_dead && g_zapCool <= 0.f) {
            std::vector<V> pf;
            for (int hd = 0; hd < 2; hd++) if (g_hand[hd].def == D_TORCH && g_hand[hd].lit) pf.push_back(handFlame[hd]);
            if (g_carrying && g_carry.def == D_TORCH && g_carry.lit) pf.push_back(looseFlamePos(g_carry));
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
            for (auto& o : g_loose) if (o.def == D_BOOK && o.burn <= 0.f) {
                V c{o.pos.x, o.pos.y + .05f, o.pos.z}; bool ig = false;
                for (auto& f : fl) if (*f.lit && dist(f.pos, c) < (f.where == 4 ? 1.2f : .85f)) { ig = true; break; }
                if (g_fireLit && o.pos.x > 3.9f && o.pos.x < 4.85f && fabsf(o.pos.z) < .9f) ig = true;
                if (tableState() == 1 && o.pos.y > .5f) ig = true;
                if (ig) { o.burn = .01f; say("Il libro prende fuoco!", 4.f); }
            }
            for (auto& o : g_loose) if (o.def == D_BOOK && o.burn > 0.f) {
                o.burn += dt;
                if (o.burn > 3.f) {                                           // si riduce in cenere tossica
                    o.def = D_ASH; o.burn = 0;
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
        for (Entity& o : g_loose) if (o.def == D_CHEST && o.open) o.openDeg = std::min(105.f, o.openDeg + 130.f * dt);   // coperchio che si solleva
        if (g_carrying && g_carry.def == D_CHEST && g_carry.open) g_carry.openDeg = std::min(105.f, g_carry.openDeg + 130.f * dt);

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
        if (tableState() == 1) lights.push_back(V{0, 1.1f, 0});                  // il tavolo in fiamme fa luce
        if (rackState() == 1) lights.push_back(V{-4.6f, 1.0f, 0});               // e la rastrelliera
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
            else if (menu == 4) hud = std::string(!menuContent() ? "Qualcosa" : menuContent()->def == D_GEM ? "Una gemma azzurra" : menuContent()->def == D_COINS ? "Un mucchio di monete d'oro" : "Un oggetto") + ". Cosa vuoi fare?\n1 – Riponi nell'inventario\n2 – Tieni in mano\nEsc – Annulla";
            else if (invOpen) {
                if (hover >= 0) {
                    if (hover >= 11) {
                        int ix = hover - 11;
                        hud = g_equip[ix] ? std::string(SLOT_NAME[ix]) + ": indossato — clic: toglilo (in mano o nell'inventario)"
                                          : std::string(SLOT_NAME[ix]) + " — vuoto: tieni in mano il pezzo e clicca qui per indossarlo";
                    } else if (hover >= 9) {
                        const Entity& it = g_hand[hover - 9]; const char* nm = hover == 9 ? "sinistra" : "destra";
                        hud = it.def ? std::string("Mano ") + nm + ": " + itemName(it) + (it.def == D_BUCKET && holds(it, D_KEY) ? " — clic: scegli cosa fare" : " — clic per riporla nell'inventario")
                                      : std::string("Mano ") + nm + " vuota";
                    } else {
                        const Entity& it = g_inv[hover];
                        if (it.def) hud = itemName(it) + (it.def == D_BUCKET && holds(it, D_KEY) ? " — clic: scegli (frugare o prenderlo in mano)" : " — clic: mano destra · clic destro: mano sinistra") + (isArmor(it.def) ? " · Ctrl+clic: indossa" : "");
                        else hud = (g_hand[0].def || g_hand[1].def) ? "Slot vuoto — clic: riponi dalla mano destra · clic destro: dalla sinistra" : "Slot vuoto";
                    }
                } else if (figHit(IL, mxp, myp)) hud = "Trascina col mouse (o frecce sinistra/destra) per ruotare la figura a 360°";
                else if (msgT > 0) hud = msg;
                else hud = "Clic: mano destra · clic destro o Maiusc+1–9: mano sinistra · I o Esc per chiudere";
            }
            else if (msgT > 0) hud = msg;
            else if (g_carrying) hud = std::string("Trasporti ") + defArticle(g_carry.def) + " — rilascia E per lasciarlo cadere";
            else if (near && igniteT > 0) hud = "Le fiamme si toccano...";
            else if (target >= 0) hud = "[E] " + hs[target].label + (movableHot(hs[target]) ? " · tieni premuto per trasportare" : "");
            else if (bucketHeldUnsearched) hud = "[E] Fruga nella cenere del secchio che tieni in mano";
            else if (g_hand[0].def == D_TORCH && g_hand[1].def == D_TORCH && g_hand[0].lit != g_hand[1].lit) hud = "Tieni premuto [F] per accostare le due torce";
            else if (g_hand[0].def || g_hand[1].def) hud = "[Maiusc+E] Lascia cadere ciò che tieni in mano";
        }
        if (!dicePhase) {                                                     // vita, veleno (clessidra) e morte
            tc.panelAt("Vita: " + std::to_string(g_hp) + " / " + std::to_string(hpMax()) + (g_poison > 0 ? "  ·  intossicato" : ""), 24, 24);
            { std::vector<Countdown> timers; if (g_poison > 0) timers.push_back({"Veleno", g_poison, 10.f}); drawCountdowns(tc, w, h, timers); }
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
