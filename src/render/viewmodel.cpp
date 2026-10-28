// Livello render — oggetti tenuti in mano (viewmodel in prima persona).
#include "render/render.h"
#include "items.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include <cmath>

// oggetto tenuto in mano (side: -1 sinistra, +1 destra); join = 0..1 avvicina le mani al centro
void drawHandItem(const Entity& it, int side, float join) {
    if (!it.def) return;
    PM _; glLoadIdentity();
    float bx = side * .30f * (1 - .7f * join);
    switch (it.def) {
        case D_TORCH:
            glTranslatef(bx, -.38f, -.60f); glRotatef(-20, 1, 0, 0); glRotatef(side * 10.f, 0, 0, 1);
            drawTorchBody(it.lit); break;
        case D_KEY:
            glTranslatef(bx * .93f, -.22f, -.55f); glScalef((float)side, 1, 1);
            glRotatef(70, 0, 0, 1); glRotatef(-25, 0, 1, 0); glRotatef(-60, 1, 0, 0);
            col(.95f, .70f, .20f); drawKey(); break;
        case D_SWORD:   // spada: impugnata con la punta in alto e in avanti
            glTranslatef(bx * .95f, -.42f, -.62f); glScalef((float)side, 1, 1);
            glRotatef(-32, 1, 0, 0); glRotatef(-8, 0, 0, 1); glScalef(.5f, .5f, .5f); glRotatef(90, 0, 0, 1);
            glTranslatef(.8f, -2.7f, -.54f); drawSword(); break;
        case D_SHIELD:   // scudo: imbracciato, rivolto in avanti
            glTranslatef(bx * 1.05f, -.30f, -.72f); glScalef((float)side, 1, 1);
            glRotatef(-18.f, 0, 1, 0); glScalef(.27f, .27f, .27f); glTranslatef(0, -2.0f, 0);
            drawShield(); break;
        case D_BUCKET:   // secchio: portato a mano, un po' più in basso
            glTranslatef(bx * .9f, -.62f, -.78f); glRotatef(15, 1, 0, 0); glScalef(.85f, .85f, .85f);
            drawBucketBody(); break;
        case D_GEM: case D_COINS: case D_BOOK: case D_HELM: case D_CUIRASS: case D_PAULDRON: case D_GREAVE: case D_VAMBRACE: case D_BOOT:   // gemma, monete, libro, pezzi d'armatura
            glTranslatef(bx * .9f, -.36f, -.70f); glRotatef(15, 1, 0, 0); glRotatef(side * 20.f, 0, 1, 0); glScalef(.6f, .6f, .6f);
            drawItemModel(it.def); break;
    }
}
