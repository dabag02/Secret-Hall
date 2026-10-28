// Definizioni dello stato condiviso (vedi state.h).
#include "state.h"

Wall W[4] = {
    {-5, -5,  1, 0,  0,  1, {{4.2f, 5.8f, 2.6f}}},   // Nord (z=-5): porta in x=0
    { 5,  5, -1, 0,  0, -1, {{4.2f, 5.8f, 2.6f}}},   // Sud  (z=+5): porta in x=0
    { 5, -5,  0, 1, -1,  0, {{7.6f, 9.2f, 2.6f}}},   // Est  (x=+5): porta in z=+3.4, camino in z=0
    {-5, -5,  0, 1,  1,  0, {{0.8f, 2.4f, 2.6f}}},   // Ovest(x=-5): porta in z=-3.4, scudo in z=0
};
const float DOOR_U[4] = {5.0f, 5.0f, 8.4f, 1.6f};
const float TORCH_U[4] = {2.2f, 2.2f, 1.6f, 8.4f};      // posizione lungo la parete i
WallTorch g_wt[4] = {{true, true}, {true, true}, {true, true}, {true, false}};   // N, S, E accese; O spenta

bool g_fireLit = false;                 // fuoco nel camino
int g_zapDoor = 0;
float g_zapT = 0, g_time = 0;

std::vector<Entity> g_loose = {
    {D_BUCKET, false, {1.6f, 0.f, -4.4f}, 0.f, 0.f, 0.f, 0.f, {Entity{D_KEY}}},                        // la chiave di bronzo è sotto la cenere
    {D_CHEST,  false, {-.2f, .9f, 0.f},   0.f, 0.f, 0.f, 0.f, {Entity{D_GEM}, Entity{D_COINS}}},        // chiuso, con gemma e monete
    {D_HELM,    false, {-3.95f, 0.f, .9f}, .6f, 0.f},          // l'elmo a terra
    {D_BOOK,   false, {1.05f, .9f, 0.f},  .25f, 0.f},             // libro aperto sul tavolo, di fronte allo scheletro
};
bool g_carrying = false;
Entity g_carry{};

// ---- arredi e supporti ----
const int RACK_TYPE[9] = {D_CUIRASS, D_PAULDRON, D_PAULDRON, D_GREAVE, D_GREAVE, D_VAMBRACE, D_VAMBRACE, D_BOOT, D_BOOT};

std::vector<Entity> makeFixtures() {
    std::vector<Entity> f;
    auto add = [&](int def) -> Entity& { f.push_back(Entity{}); f.back().def = def; return f.back(); };
    add(D_TABLE); add(D_CHAIR); add(D_SKELETON);
    Entity& rack = add(D_RACK);
    for (int i = 0; i < 9; i++) { Entity p; p.def = RACK_TYPE[i]; p.slot = i; rack.contents.push_back(p); }
    Entity s; s.def = D_SWORD; s.slot = 0; add(D_SWORD_HOOKS).contents.push_back(s);
    Entity sh; sh.def = D_SHIELD; sh.slot = 0; add(D_SHIELD_HOOKS).contents.push_back(sh);
    return f;
}
std::vector<Entity> g_fixtures = makeFixtures();

Entity& fixture(int def) {
    for (Entity& e : g_fixtures) if (e.def == def) return e;
    static Entity none; none = Entity{};                  // non dovrebbe mai accadere: arredo mancante (errore nei dati)
    return none;
}

int hungIndex(const Entity& support, int slot) {
    for (int i = 0; i < (int)support.contents.size(); i++) if (support.contents[i].slot == slot) return i;
    return -1;
}
void hang(Entity& support, Entity item, int slot) {
    item.slot = slot; item.pos = V{0, 0, 0}; item.hot = 0; item.vy = 0;
    auto at = support.contents.begin();                   // si inserisce in ordine di posto: l'ordine degli appesi non dipende dalla storia
    while (at != support.contents.end() && at->slot < slot) ++at;
    support.contents.insert(at, item);
}
Entity unhang(Entity& support, int index) {
    Entity it = support.contents[index];
    support.contents.erase(support.contents.begin() + index);
    it.slot = -1;
    return it;
}
int spillSupport(Entity& support, std::vector<Entity>& world, const V* slotPos, float hot) {
    int n = 0;
    for (Entity it : support.contents) {
        int slot = it.slot;
        it.slot = -1; it.pos = slotPos[slot]; it.yaw = slot * .7f; it.vy = 0; it.hot = hot;
        world.push_back(it); n++;
    }
    support.contents.clear();
    return n;
}
