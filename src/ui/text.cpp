// Interfaccia — testo a schermo (SDL_ttf -> texture OpenGL).
#include "ui/text.h"
#include "ui/draw2d.h"
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

static std::string findFont() {
    auto exists = [](const std::string& p) { FILE* f = fopen(p.c_str(), "rb"); if (f) fclose(f); return f != nullptr; };
    if (const char* e = getenv("FONT")) if (exists(e)) return e;
    const char* names[] = {"Book Antiqua", "Palatino Linotype", "Palatino", "P052", "TeX Gyre Pagella", "Palladio"};
    for (const char* n : names) {            // fontconfig: accetta solo se la famiglia trovata è davvero "palatina"
        std::string cmd = std::string("fc-match -f '%{family}|%{file}' '") + n + "' 2>/dev/null";
        FILE* p = popen(cmd.c_str(), "r"); if (!p) continue;
        char buf[1024] = {0}; size_t k = fread(buf, 1, sizeof(buf) - 1, p); pclose(p); buf[k] = 0;
        std::string r = buf, fam = r.substr(0, r.find('|')), file = r.find('|') == std::string::npos ? "" : r.substr(r.find('|') + 1);
        for (auto& c : fam) c = (char)tolower((unsigned char)c);
        for (const char* key : {"antiqua", "palatino", "p052", "pagella", "palladio"})
            if (fam.find(key) != std::string::npos && exists(file)) return file;
    }
    const char* paths[] = {
        "/usr/share/fonts/urw-base35/P052-Roman.otf", "/usr/share/fonts/opentype/urw-base35/P052-Roman.otf",
        "/usr/share/fonts/OTF/texgyrepagella-regular.otf", "/usr/share/fonts/opentype/tex-gyre/texgyrepagella-regular.otf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", "/usr/share/fonts/TTF/DejaVuSerif.ttf",
        "/usr/share/fonts/dejavu/DejaVuSerif.ttf"};
    for (const char* p : paths) if (exists(p)) return p;
    return "";
}

void TextCache::init() {
    if (TTF_Init() != 0) { fprintf(stderr, "SDL_ttf: %s\n", TTF_GetError()); return; }
    std::string path = findFont();
    if (path.empty()) { fprintf(stderr, "Nessun font trovato: i messaggi andranno su terminale.\n"); return; }
    // corpo 14 pt a 96 dpi = 18,67 px (SDL_ttf usa i punti a 72 dpi, quindi 14*96/72 ≈ 19)
    font = (void*)TTF_OpenFont(path.c_str(), 19);
    if (!font) fprintf(stderr, "Font %s: %s\n", path.c_str(), TTF_GetError());
    else fprintf(stderr, "Font: %s\n", path.c_str());
}

TextCache::E* TextCache::get(const std::string& txt, int wrap) {
    if (!font || txt.empty()) return nullptr;
    std::string key = std::to_string(wrap) + "|" + txt;
    auto it = m.find(key);
    if (it != m.end()) return it->second.tex ? &it->second : nullptr;
    E e; SDL_Color white{255, 255, 255, 255};
    std::string shaped;                                   // il font P052 fonde "ce" in una legatura "œ" (in certi contesti): uno spazio sottile (U+200A) la impedisce
    for (size_t i = 0; i < txt.size(); i++) { shaped += txt[i]; if (txt[i] == 'c' && i + 1 < txt.size() && txt[i + 1] == 'e') shaped += "\xE2\x80\x8A"; }
    SDL_Surface* s0 = TTF_RenderUTF8_Blended_Wrapped((TTF_Font*)font, shaped.c_str(), white, (Uint32)wrap);
    if (s0) {
        SDL_Surface* s1 = SDL_ConvertSurfaceFormat(s0, SDL_PIXELFORMAT_ABGR8888, 0);   // byte R,G,B,A
        SDL_FreeSurface(s0);
        if (s1) {
            glGenTextures(1, &e.tex); glBindTexture(GL_TEXTURE_2D, e.tex);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, s1->pitch / 4);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s1->w, s1->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s1->pixels);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            e.w = s1->w; e.h = s1->h; SDL_FreeSurface(s1);
        }
    }
    E& ref = m[key] = e;
    return ref.tex ? &ref : nullptr;
}

void TextCache::draw(const std::string& txt, float x, float y, int wrap) {   // origine in alto a sinistra
    E* e = get(txt, wrap); if (!e) return;
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, e->tex); glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y); glTexCoord2f(1, 0); glVertex2f(x + e->w, y);
    glTexCoord2f(1, 1); glVertex2f(x + e->w, y + e->h); glTexCoord2f(0, 1); glVertex2f(x, y + e->h);
    glEnd(); glDisable(GL_TEXTURE_2D);
}

void TextCache::shutdown() {
    for (auto& kv : m) if (kv.second.tex) glDeleteTextures(1, &kv.second.tex);
    if (font) TTF_CloseFont((TTF_Font*)font);
    if (TTF_WasInit()) TTF_Quit();
}

void TextCache::panel(const std::string& txt, int w, int h) {
    const int pad = 16, margin = 28;
    E* e = get(txt, std::max(100, std::min(w - 60, 960) - 2 * pad));
    if (!e) return;
    float pw = (float)(e->w + 2 * pad), ph = (float)(e->h + 2 * pad), x0 = (w - pw) / 2.f, y0 = h - ph - margin;
    fillRect(x0, y0, pw, ph, 0, 0, 0, .70f);                  // sfondo nero semitrasparente
    strokeRect(x0, y0, pw, ph, 1, 1, 1, .28f);
    draw(txt, x0 + pad, y0 + pad, std::max(100, std::min(w - 60, 960) - 2 * pad));
}
void TextCache::panelAt(const std::string& txt, float x, float y) {
    const int pad = 14;
    E* e = get(txt, 0);
    if (!e) return;
    fillRect(x, y, (float)(e->w + 2 * pad), (float)(e->h + 2 * pad), 0, 0, 0, .70f);
    strokeRect(x, y, (float)(e->w + 2 * pad), (float)(e->h + 2 * pad), 1, 1, 1, .28f);
    draw(txt, x + pad, y + pad, 0);
}
