// Clessidra: simbolo globale di ogni conto alla rovescia di un evento limitato nel tempo (regola P1).
#pragma once
#include <string>
#include <vector>
#include "ui/text.h"

struct Countdown { std::string label; float remaining, total; };   // etichetta, secondi rimasti, durata totale

// Disegna una clessidra animata (sabbia che scende) per ogni conto alla rovescia, in basso a destra, da destra verso sinistra.
void drawCountdowns(TextCache& tc, int w, int h, const std::vector<Countdown>& timers);
