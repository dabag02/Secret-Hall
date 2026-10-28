// Primitive wireframe (livello gfx): linee, scatole, coni, cupole, anelli. Usano OpenGL a pipeline fissa.
// Nessuna logica di gioco qui dentro: solo geometria e un piccolo stato di colore (tinta "rovente").
#include "gfx/primitives.h"
#include <algorithm>
#include <cmath>

bool g_tintHot = false;       // oggetti roventi: colore pulsante rosso-arancio
float g_hotPulse = 0;         // fase della pulsazione (aggiornata dal ciclo di gioco)

// le linee lunghe vengono spezzate (<= 0,5 m) perché l'illuminazione è calcolata per vertice
void line(V a, V b) {
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    int n = std::max(1, (int)ceilf(sqrtf(dx * dx + dy * dy + dz * dz) / .5f));
    glBegin(GL_LINES);
    for (int i = 0; i < n; i++) {
        float t0 = (float)i / n, t1 = (float)(i + 1) / n;
        glVertex3f(a.x + dx * t0, a.y + dy * t0, a.z + dz * t0);
        glVertex3f(a.x + dx * t1, a.y + dy * t1, a.z + dz * t1);
    }
    glEnd();
}
void line(float x1, float y1, float z1, float x2, float y2, float z2) {
    line(V{x1, y1, z1}, V{x2, y2, z2});
}
void strip(const std::vector<V>& p, bool closed) {
    glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (auto& q : p) glVertex3f(q.x, q.y, q.z);
    glEnd();
}
// arco di ellisse: c + a*cos(t) + b*sin(t)
void arc(V c, V a, V b, float t0, float t1, int n) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= n; i++) {
        float t = t0 + (t1 - t0) * i / n, cs = cosf(t), sn = sinf(t);
        glVertex3f(c.x + a.x * cs + b.x * sn, c.y + a.y * cs + b.y * sn, c.z + a.z * cs + b.z * sn);
    }
    glEnd();
}
void ringXZ(V c, float rx, float rz, int n) { arc(c, {rx, 0, 0}, {0, 0, rz}, 0, 2 * PI, n); }
void ringXY(V c, float rx, float ry, int n) { arc(c, {rx, 0, 0}, {0, ry, 0}, 0, 2 * PI, n); }
void ringYZ(V c, float ry, float rz, int n) { arc(c, {0, ry, 0}, {0, 0, rz}, 0, 2 * PI, n); }

void rect(float x0, float y0, float x1, float y1, float z) {
    strip({{x0, y0, z}, {x1, y0, z}, {x1, y1, z}, {x0, y1, z}}, true);
}
// parallelepipedo centrato nell'origine
void box(float w, float h, float d) {
    float x = w / 2, y = h / 2, z = d / 2;
    for (int sy = -1; sy <= 1; sy += 2) {
        line(-x, sy * y, -z, x, sy * y, -z); line(x, sy * y, -z, x, sy * y, z);
        line(x, sy * y, z, -x, sy * y, z);   line(-x, sy * y, z, -x, sy * y, -z);
    }
    line(-x, -y, -z, -x, y, -z); line(x, -y, -z, x, y, -z);
    line(x, -y, z, x, y, z);     line(-x, -y, z, -x, y, z);
}
void boxAt(float cx, float cy, float cz, float w, float h, float d) {
    PM _; glTranslatef(cx, cy, cz); box(w, h, d);
}
// tronco di cono lungo +Y, base nell'origine
void cone(float r0, float r1, float h, int seg, int vert) {
    if (vert < 0) vert = seg;
    arc({0, 0, 0}, {r0, 0, 0}, {0, 0, r0}, 0, 2 * PI, seg);
    arc({0, h, 0}, {r1, 0, 0}, {0, 0, r1}, 0, 2 * PI, seg);
    glBegin(GL_LINES);
    for (int i = 0; i < vert; i++) {
        float t = 2 * PI * i / vert, c = cosf(t), s = sinf(t);
        glVertex3f(r0 * c, 0, r0 * s); glVertex3f(r1 * c, h, r1 * s);
    }
    glEnd();
}
// cupola/sfera ellissoidale tra le latitudini p0..p1 (radianti)
void dome(float rx, float ry, float rz, float p0, float p1, int nLat, int nLon) {
    for (int i = 0; i <= nLat; i++) {
        float p = p0 + (p1 - p0) * i / nLat, c = cosf(p);
        if (c < 0.02f) continue;
        arc({0, ry * sinf(p), 0}, {rx * c, 0, 0}, {0, 0, rz * c}, 0, 2 * PI, nLon * 2);
    }
    for (int j = 0; j < nLon; j++) {
        float t = 2 * PI * j / nLon, ct = cosf(t), st = sinf(t);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= nLat * 2; i++) {
            float p = p0 + (p1 - p0) * i / (nLat * 2), c = cosf(p);
            glVertex3f(rx * c * ct, ry * sinf(p), rz * c * st);
        }
        glEnd();
    }
}
void sphere(float rx, float ry, float rz, int nLat, int nLon) {
    dome(rx, ry, rz, -PI / 2, PI / 2, nLat, nLon);
}
// cilindro/cono disteso lungo l'asse X, centrato in (cx,cy,cz)
void coneAlongX(float cx, float cy, float cz, float r0, float r1, float len, int seg) {
    PM _; glTranslatef(cx, cy, cz); glRotatef(-90, 0, 0, 1); glTranslatef(0, -len / 2, 0);
    cone(r0, r1, len, seg);
}
