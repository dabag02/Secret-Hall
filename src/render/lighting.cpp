// Livello render — Illuminazione (luci ambientali con attenuazione, per vertice) e matrice della camera.
#include "render/render.h"
#include "state.h"
#include "core/config.h"
#include "gfx/primitives.h"
#include "skeleton_physics.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// Luce solo "ambientale" con attenuazione quadratica: il colore di ogni vertice viene scalato dalla vicinanza alle fiamme.
void setLights(const std::vector<V>& pos) {
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

void lookAt(V eye, V f) {
    float fl = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z); f = {f.x / fl, f.y / fl, f.z / fl};
    V s{-f.z, 0, f.x}; float sl = sqrtf(s.x * s.x + s.z * s.z); s = {s.x / sl, 0, s.z / sl};
    V u{s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x};
    float m[16] = {s.x, u.x, -f.x, 0,  s.y, u.y, -f.y, 0,  s.z, u.z, -f.z, 0,  0, 0, 0, 1};
    glMultMatrixf(m); glTranslatef(-eye.x, -eye.y, -eye.z);
}
