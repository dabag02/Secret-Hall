// Testo a schermo: stringhe UTF-8 -> texture OpenGL (SDL_ttf), con cache; riquadri semitrasparenti per messaggi e pannelli.
// Font: ricerca automatica (Book Antiqua/Palatino/P052/TeX Gyre Pagella, poi DejaVu Serif); variabile FONT per forzarlo.
#pragma once
#include <SDL2/SDL_opengl.h>
#include <map>
#include <string>


struct TextCache {
    struct E { GLuint tex = 0; int w = 0, h = 0; };
    void* font = nullptr;                                 // TTF_Font* di SDL_ttf, opaco: il nome della struct cambia tra le versioni di SDL_ttf (_TTF_Font / TTF_Font)
    std::map<std::string, E> m;

    void init();                                          // apre il font (corpo 14 pt a 96 dpi)
    E* get(const std::string& txt, int wrap);             // texture del testo (in cache); wrap = larghezza massima in px, 0 = solo a capo espliciti
    void draw(const std::string& txt, float x, float y, int wrap = 0);   // origine in alto a sinistra
    void panel(const std::string& txt, int w, int h);     // riquadro in basso, centrato
    void panelAt(const std::string& txt, float x, float y);   // riquadro in un punto qualsiasi
    void shutdown();
};
