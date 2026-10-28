// Primitive wireframe (livello gfx). Tutte le funzioni disegnano in modalità immediata con la matrice corrente.
// Convenzione: Y verso l'alto; i solidi sono centrati nell'origine salvo indicazione.
#pragma once
#include <SDL2/SDL_opengl.h>
#include <vector>
#include "core/types.h"

// Salva/ripristina la matrice corrente (RAII): `PM _;` all'inizio di un blocco.
struct PM { PM() { glPushMatrix(); } ~PM() { glPopMatrix(); } };

// Tinta "rovente": quando g_tintHot è vero ogni colore diventa un rosso-arancio pulsante (g_hotPulse in 0..1).
extern bool g_tintHot;
extern float g_hotPulse;
inline void col(float r, float g, float b) { if (g_tintHot) glColor3f(1.f, .28f + .18f * g_hotPulse, .06f); else glColor3f(r, g, b); }

// Le linee lunghe vengono spezzate (<= 0,5 m) perché l'illuminazione è calcolata per vertice.
void line(V a, V b);
void line(float x1, float y1, float z1, float x2, float y2, float z2);
void strip(const std::vector<V>& p, bool closed = false);                 // polilinea (chiusa o aperta)
void arc(V c, V a, V b, float t0, float t1, int n);                       // arco di ellisse: c + a*cos(t) + b*sin(t)
void ringXZ(V c, float rx, float rz, int n = 16);                         // anelli nei tre piani
void ringXY(V c, float rx, float ry, int n = 16);
void ringYZ(V c, float ry, float rz, int n = 16);
void rect(float x0, float y0, float x1, float y1, float z);               // rettangolo nel piano XY a quota z
void box(float w, float h, float d);                                      // parallelepipedo centrato nell'origine
void boxAt(float cx, float cy, float cz, float w, float h, float d);
void cone(float r0, float r1, float h, int seg = 12, int vert = -1);      // tronco di cono lungo +Y, base nell'origine
void dome(float rx, float ry, float rz, float p0, float p1, int nLat, int nLon);   // cupola/sfera ellissoidale tra le latitudini p0..p1
void sphere(float rx, float ry, float rz, int nLat = 4, int nLon = 8);
void coneAlongX(float cx, float cy, float cz, float r0, float r1, float len, int seg = 8);   // cono disteso lungo X, centrato
