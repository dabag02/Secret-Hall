// Interfaccia — figura umana in wireframe.
#include "ui/figure.h"
#include "items.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "render/render.h"
#include "ui/draw2d.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

void limb(V a, V b, float r0, float r1, int n) {       // tronco di cono tra due punti qualsiasi
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
void drawMan() {
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
