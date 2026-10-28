// Livello render — Scheletro seduto: disegno del corpo; la posa delle braccia viene da skeleton_physics.
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void bone(V a, V b, float r) {
    line(a, b); ringXZ(a, r, r, 6); ringXZ(b, r, r, 6);
}

void drawSkeleton() {
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
