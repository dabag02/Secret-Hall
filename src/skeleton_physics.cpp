// Fisica dello scheletro (logica, senza grafica). Peso: ossa ~9 kg (10,8 con l'elmo), vedi SKELETON_KG in core/config.h.
// Le braccia sono un doppio pendolo (omero e avambraccio+mano): appoggiate al tavolo finché esiste; quando il tavolo
// brucia e crolla, cadono ai fianchi per gravità, con smorzamento.
#include "skeleton_physics.h"
#include "state.h"
#include "core/config.h"
#include <algorithm>
#include <cmath>

static const float ARM_L1 = .30f, ARM_L2 = .25f, ARM_M1 = .6f, ARM_M2 = .5f;     // lunghezze (m) e masse (kg)
struct ArmState { float t1, t2, w1, w2; };                                        // angoli dalla verticale (rad), + = verso il tavolo
static ArmState g_arm[2];

void armPose(int sd, V& sh, V& el, V& wr) {
    const float a = 12.f * PI / 180.f;
    sh = {2.05f - .5f * sinf(a), .60f + .5f * cosf(a), sd * .19f};
    if (!fixture(D_SKELETON).armsFree) {                      // mano appoggiata sul bordo del tavolo: cinematica inversa a due segmenti
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

void updateSkeleton(float dt) {
    Entity& skel = fixture(D_SKELETON);
    if (tableState() == 2 && !skel.armsFree) {            // viene a mancare l'appoggio: le braccia si staccano
        for (int h = 0; h < 2; h++) {
            V sh, el, wr; armPose(h ? 1 : -1, sh, el, wr);   // posa corrente (ancora a riposo) = condizione iniziale
            g_arm[h] = {atan2f(-(el.x - sh.x), -(el.y - sh.y)), atan2f(-(wr.x - el.x), -(wr.y - el.y)), 0.f, 0.f};
        }
        skel.armsFree = true;
    }
    if (!skel.armsFree) return;
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
