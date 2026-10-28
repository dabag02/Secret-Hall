// Funzioni di disegno (livello render). Passo M2: dichiarazioni condivise tra i file di render/ e il resto del gioco.
// Dal passo M7 il gioco non le chiamerà più direttamente: la logica produrrà una RenderScene e un backend la disegnerà.
#pragma once
#include <vector>
#include "core/types.h"
#include "state.h"
#include "items.h"

// --- render/room.cpp: Stanza: pareti a blocchi, pavimento, travi, porte, quadri e scarica delle porte magiche.
void wallFrame(const Wall& w, float u);
void wallStones(const Wall& w);
void drawFloor();
void drawCeiling();
void drawDoor();
void drawPainting(int variant);
void drawZap();

// --- render/wall_props.cpp: Oggetti appesi alle pareti: camino, spada, scudo (con i loro ganci) e torce a muro.
void drawFireplace();
void drawSword();
void drawSwordHooks();
std::vector<V> shieldOutline(float s);
void drawShield();
void drawShieldHooks();
void drawWallTorches();

// --- render/models.cpp: Modelli di oggetti piccoli (origine locale): elmo e pezzi d'armatura, gemma, monete, libro, cenere, chiave, fiamma, torcia, secchio.
void drawHelm(float r);
void drawCuirassModel();
void drawPauldronModel();
void drawGreaveModel();
void drawVambraceModel();
void drawBootModel();
void drawGemModel();
void drawBookModel();
void drawAshModel();
void drawCoinsModel();
void drawFlame(float phase = 0);
void drawTorchBody(bool lit, bool withFlame = true);
void drawKey();
void drawBucketBody();
void drawArmorLying(int t);

// --- render/rack.cpp: Rastrelliera dell'armatura: integra, in fiamme e carbonizzata.
void drawRack();
void drawRackFire();
void drawRackRemains();

// --- render/furniture.cpp: Arredi: tavolo (integro, in fiamme, bruciato), sedia, scrigno.
void drawTable();
void drawChair();
void drawChest(const Entity& chest);   // corpo, coperchio (openDeg), lucchetto o contenuto
void drawTableFire();
void drawTableRemains();

// --- render/skeleton.cpp: Scheletro seduto: disegno del corpo; la posa delle braccia viene da skeleton_physics.
void bone(V a, V b, float r = .018f);
void drawSkeleton();

// --- render/scene.cpp: Scena: oggetti sciolti e trasportati, e disegno dell'intera sala (drawWorld).
void drawLooseObj(const Entity& o);
void drawLooseObjects();
void drawWorld();

// --- render/lighting.cpp: Illuminazione (luci ambientali con attenuazione, per vertice) e matrice della camera.
void setLights(const std::vector<V>& pos);
void lookAt(V eye, V f);

// --- render/models.cpp (inventario): modello normalizzato (~0,45 u) per icone e mano
void drawItemModel(int type);

// --- render/viewmodel.cpp: oggetto tenuto in mano (side: -1 sinistra, +1 destra; join 0..1 avvicina le mani)
void drawHandItem(const Entity& it, int side, float join);
