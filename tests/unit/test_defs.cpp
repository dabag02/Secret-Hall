// Test headless (nessun SDL/GL) di definizioni ed entità (M4a/M4b, regola J1).
// I valori attesi sono quelli di v0.7 (changelog.md, "Regole di gioco già decise"): nomi, articoli e pesi non devono cambiare.
#include "items.h"
#include <cstdio>
#include <cstring>
#include <string>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FALLITO: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

struct Gold { int def; const char* name; const char* nameLit; const char* article; float kg; bool inventory; };
static const Gold GOLD[] = {
    {D_KEY,      "Chiave di bronzo",       nullptr,         "la chiave",          .15f, true},
    {D_TORCH,    "Torcia spenta",          "Torcia accesa", "la torcia",          1.2f, true},
    {D_SWORD,    "Spada lunga",            nullptr,         "la spada",           1.6f, true},
    {D_SHIELD,   "Scudo con croce dorata", nullptr,         "lo scudo",           3.0f, true},
    {D_BUCKET,   "Secchio di legno",       nullptr,         "il secchio",         3.0f, true},
    {D_GEM,      "Gemma",                  nullptr,         "la gemma",           .1f,  true},
    {D_COINS,    "Monete d'oro",           nullptr,         "le monete d'oro",    .9f,  true},
    {D_BOOK,     "Libro",                  nullptr,         "il libro",           1.2f, true},
    {D_HELM,     "Elmo",                   nullptr,         "l'elmo",             1.8f, true},
    {D_CUIRASS,  "Corazza",                nullptr,         "la corazza",         5.0f, true},
    {D_PAULDRON, "Spallina",               nullptr,         "una spallina",       1.0f, true},
    {D_GREAVE,   "Gambale",                nullptr,         "un gambale",         1.5f, true},
    {D_VAMBRACE, "Bracciale",              nullptr,         "un bracciale",       .8f,  true},
    {D_BOOT,     "Stivale",                nullptr,         "uno stivale",        1.2f, true},
    {D_CHEST,    "Scrigno",                nullptr,         "lo scrigno",         12.f, false},
    {D_ASH,      "Ceneri tossiche",        nullptr,         "le ceneri tossiche", 0.f,  false},
};

int main() {
    // 1) nomi, articoli, pesi, inventariabilità
    for (const Gold& g : GOLD) {
        const ObjectDef& d = defOf(g.def);
        CHECK(d.id == g.def, "defOf(%d) non trovato", g.def);
        CHECK(!strcmp(d.name, g.name), "nome di %d: '%s' invece di '%s'", g.def, d.name, g.name);
        CHECK((d.nameLit == nullptr) == (g.nameLit == nullptr) && (!g.nameLit || !strcmp(d.nameLit, g.nameLit)), "nome da acceso di %d", g.def);
        CHECK(!strcmp(defArticle(g.def), g.article), "articolo di %d: '%s' invece di '%s'", g.def, defArticle(g.def), g.article);
        CHECK(itemKg(g.def) == g.kg, "peso di %d: %.2f invece di %.2f", g.def, itemKg(g.def), g.kg);
        CHECK(defHas(g.def, F_INVENTORY) == g.inventory, "inventariabilità di %d", g.def);
        Entity e; e.def = g.def;
        CHECK(itemName(e) == (g.inventory ? g.name : ""), "itemName(%d)", g.def);
        if (g.nameLit) { e.lit = true; CHECK(itemName(e) == g.nameLit, "itemName acceso(%d)", g.def); }
    }
    {   // il nome del secchio dipende dal suo contenuto (la chiave sotto la cenere), non da un flag globale
        Entity b; b.def = D_BUCKET;
        CHECK(itemName(b) == "Secchio di legno", "secchio vuoto");
        b.contents.push_back(Entity{D_KEY});
        CHECK(itemName(b) == "Secchio di cenere (da frugare)", "secchio con la chiave");
        Entity inSlot = slotOf(b);
        CHECK(itemName(inSlot) == "Secchio di cenere (da frugare)" && holds(inSlot, D_KEY), "il contenuto viaggia con il secchio (inventario/mano)");
        b.contents.erase(b.contents.begin() + contentIndex(b, D_KEY));
        CHECK(itemName(b) == "Secchio di legno" && !holds(b, D_KEY) && inSlot.contents.size() == 1, "svuotare un secchio non tocca la copia");
    }
    {   // scrigno: contenuto = oggetti veri, nell'ordine gemma, monete; una copia (trasporto) porta con sé contenuto e stato
        Entity c; c.def = D_CHEST; c.contents = {Entity{D_GEM}, Entity{D_COINS}}; c.open = true; c.openDeg = 50;
        Entity carried = c;
        CHECK(carried.open && carried.openDeg == 50 && carried.contents.size() == 2 && contentIndex(carried, D_COINS) == 1, "copia dello scrigno");
        carried.contents.erase(carried.contents.begin());
        CHECK(c.contents.size() == 2 && contentIndex(carried, D_GEM) == -1, "le copie sono indipendenti");
    }
    CHECK(!strcmp(defArticle(D_NONE), "il pezzo"), "articolo di D_NONE");

    // 2) l'armatura completa pesa 15,8 kg (1,8 + 5 + 2·1 + 2·1,5 + 2·0,8 + 2·1,2) e ogni slot accetta un pezzo indossabile
    float armor = 0;
    for (int i = 0; i < 10; i++) { armor += itemKg(SLOT_TYPE[i]); CHECK(isArmor(SLOT_TYPE[i]), "slot %d non indossabile", i); }
    CHECK(armor > 15.79f && armor < 15.81f, "armatura completa %.2f kg (attesi 15,8)", armor);
    CHECK(!isArmor(D_SWORD) && !isArmor(D_CHEST) && !isArmor(D_NONE), "isArmor");

    // 3) slotOf: da oggetto del mondo a oggetto da mano/inventario
    {
        Entity w; w.def = D_TORCH; w.lit = true; w.pos = {1, 2, 3}; w.hot = 5; w.burn = 7;
        Entity s = slotOf(w);
        CHECK(s.def == D_TORCH && s.lit && s.pos.x == 0 && s.hot == 0 && s.burn == 0, "slotOf(torcia accesa) deve tenere solo def, lit e contenuto");
        Entity sw; sw.def = D_SWORD; sw.lit = true; CHECK(!slotOf(sw).lit, "solo le torce hanno lo stato acceso");
        Entity ch; ch.def = D_CHEST; CHECK(slotOf(ch).def == D_NONE, "lo scrigno non entra nell'inventario");
        Entity as; as.def = D_ASH;   CHECK(slotOf(as).def == D_NONE, "le ceneri non entrano nell'inventario");
        Entity bk; bk.def = D_BUCKET; bk.contents.push_back(Entity{D_KEY}); CHECK(slotOf(bk).contents.size() == 1, "slotOf mantiene il contenuto");
        Entity ar; ar.def = D_CUIRASS; CHECK(slotOf(ar).def == D_CUIRASS, "i pezzi d'armatura restano se stessi (niente più `sub`)");
        CHECK(slotOf(Entity{}).def == D_NONE, "slotOf(vuoto)");
    }
    // 4) peso totale = inventario + mani + indossato + oggetto trasportato
    {
        for (auto& s : g_inv) s = Entity{};
        for (auto& h : g_hand) h = Entity{};
        for (bool& b : g_equip) b = false;
        g_carrying = false;
        g_inv[0].def = D_KEY; g_hand[1].def = D_TORCH; g_equip[1] = true;           // chiave + torcia in mano + corazza indossata
        CHECK(totalKg() > 6.34f && totalKg() < 6.36f, "totalKg %.3f (attesi 6,35)", totalKg());
        g_carrying = true; g_carry.def = D_CHEST;
        CHECK(totalKg() > 18.34f && totalKg() < 18.36f, "totalKg con scrigno %.3f (attesi 18,35)", totalKg());
        g_carrying = false;
    }
    // 4b) arredi e supporti (M4c, tranche 2): configurazione originale
    {
        g_fixtures = makeFixtures();
        CHECK(tableState() == 0 && rackState() == 0, "tavolo e rastrelliera partono integri");
        CHECK(!fixture(D_SKELETON).armsFree, "le braccia dello scheletro partono sostenute dal tavolo");
        CHECK(fixture(D_SKELETON).def == D_SKELETON && itemKg(D_SKELETON) == 9.0f, "scheletro: 9 kg");
        CHECK(swordOnWall() && shieldOnWall(), "spada e scudo partono appesi");
        Entity& rack = fixture(D_RACK);
        CHECK(rack.contents.size() == 9, "9 pezzi sulla rastrelliera");
        for (int i = 0; i < 9; i++) CHECK(rackHas(i) && rack.contents[hungIndex(rack, i)].def == RACK_TYPE[i], "pezzo %d sulla rastrelliera", i);
        for (int d : {D_TABLE, D_CHAIR, D_SKELETON, D_RACK, D_SWORD_HOOKS, D_SHIELD_HOOKS}) CHECK(defHas(d, F_FIXED) && !defHas(d, F_INVENTORY), "%d è un arredo fisso", d);
        // staccare un pezzo lascia gli altri ai loro posti (le due spalline restano distinguibili)
        Entity taken = unhang(rack, hungIndex(rack, 1));
        CHECK(taken.def == D_PAULDRON && taken.slot == -1 && !rackHas(1) && rackHas(2) && rack.contents.size() == 8, "unhang del posto 1");
        // riappendere
        hang(rack, taken, 1);
        CHECK(rackHas(1) && rack.contents.size() == 9, "hang sul posto 1");
        bool sorted = true; for (int i = 0; i < 9; i++) sorted = sorted && rack.contents[i].slot == i;
        CHECK(sorted, "gli appesi restano in ordine di posto");
        // crollo: tutto ciò che è appeso cade nel mondo, rovente, nella posizione del suo posto
        Entity cuir = unhang(rack, hungIndex(rack, 0));                 // un pezzo già preso non cade
        V pos[9]; for (int i = 0; i < 9; i++) pos[i] = V{(float)i, 0.f, 1.f};
        std::vector<Entity> world;
        int n = spillSupport(rack, world, pos, 10.f);
        CHECK(n == 8 && world.size() == 8 && rack.contents.empty(), "spill: 8 pezzi caduti, supporto vuoto");
        bool ok = true; for (const Entity& e : world) ok = ok && e.hot == 10.f && e.slot == -1 && e.def != D_NONE && isArmor(e.def);
        CHECK(ok, "i pezzi caduti sono rossi e non più appesi");
        CHECK(world[0].pos.x == 1.f && world[0].def == D_PAULDRON && world[7].pos.x == 8.f, "spill: ordine e posizioni dei posti");
        (void)cuir;
        // spada e scudo: appendere e staccare
        Entity& hooks = fixture(D_SWORD_HOOKS);
        Entity sw = unhang(hooks, 0);
        CHECK(sw.def == D_SWORD && !swordOnWall() && shieldOnWall(), "spada staccata, scudo ancora appeso");
        hang(hooks, sw, 0); CHECK(swordOnWall(), "spada riappesa");
        g_fixtures = makeFixtures();
    }
    // 5) coerenza della tabella
    for (int d = 1; d < D_COUNT; d++) {
        const ObjectDef& o = defOf(d);
        if (o.id != d) continue;                                   // posti liberi
        CHECK(o.key && *o.key && o.name && o.article && o.visualId && *o.visualId, "definizione %d incompleta", d);
        CHECK(o.material != M_NONE, "definizione %s senza materiale", o.key);
        CHECK(!(o.flags & F_WEARABLE) || (o.flags & F_INVENTORY), "%s indossabile ma non inventariabile", o.key);
        for (int e = d + 1; e < D_COUNT; e++) if (defOf(e).id == e) CHECK(strcmp(o.key, defOf(e).key) != 0, "chiave duplicata: %s", o.key);
    }
    CHECK(defOf(999).id == D_NONE && defOf(-3).id == D_NONE && defOf(17).id == D_NONE && defOf(9).id == D_NONE, "id sconosciuti devono dare D_NONE");
    CHECK(materialOf(M_WOOD).flammable && !materialOf(M_STEEL).flammable, "infiammabilità dei materiali");

    if (fails) { printf("%d controlli falliti\n", fails); return 1; }
    printf("test_defs: tutti i controlli superati\n");
    return 0;
}
