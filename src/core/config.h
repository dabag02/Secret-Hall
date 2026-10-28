// Costanti condivise (livello core). Valori iniziali di gioco: in futuro verranno letti da file dati (passo M11).
#pragma once

inline constexpr float WL = 10.0f, WH = 4.0f;                  // lato e altezza della sala (m); x,z in [-5,5]
inline constexpr float PR = .25f, LIM = 5.f - .35f;            // raggio del giocatore e limite di movimento dai muri
inline constexpr float MAX_KG = 20.f;                          // peso massimo trasportabile (kg)
inline constexpr float TORCH_Y = 1.9f, TORCH_Z = .10f, TORCH_TILT = 25.f;   // attacco delle torce a muro: quota, distanza, inclinazione (°)
inline constexpr float SKELETON_KG = 9.0f, SKELETON_HELM_KG = 1.8f;     // peso delle ossa e dell'elmo dello scheletro (kg)
