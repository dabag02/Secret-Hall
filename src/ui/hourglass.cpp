// Interfaccia — clessidra dei conti alla rovescia.
#include "ui/hourglass.h"
#include "state.h"
#include "gfx/primitives.h"
#include "ui/draw2d.h"
#include <algorithm>
#include <cmath>

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

void drawCountdowns(TextCache& tc, int w, int h, const std::vector<Countdown>& timers) {
    for (size_t i = 0; i < timers.size(); i++) {
        const Countdown& c = timers[i];
        float frac = c.total > 0 ? std::max(0.f, std::min(1.f, c.remaining / c.total)) : 0.f;
        drawHourglass(tc, w - 70.f - 90.f * (float)i, h - 130.f, 100.f, frac);
    }
}
