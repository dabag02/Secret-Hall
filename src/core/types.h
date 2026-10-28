// Tipi di base (livello core): nessuna dipendenza da OpenGL o SDL.
#pragma once
#include <cmath>

inline constexpr float PI = 3.14159265358979f;

struct V { float x, y, z; };                                   // vettore/punto 3D (Y verso l'alto)

inline V vadd(V a, V b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V vmul(V a, float k) { return {a.x * k, a.y * k, a.z * k}; }
inline V vcross(V a, V b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline V vnorm(V a) { float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); return l > 1e-6f ? vmul(a, 1 / l) : V{0, 1, 0}; }
