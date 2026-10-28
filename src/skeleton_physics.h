// Fisica dello scheletro (logica).
#pragma once
#include "core/types.h"

void armPose(int sd, V& sh, V& el, V& wr);   // posa delle braccia: appoggiate al tavolo (IK) o libere (pendolo); sd = -1/+1
void updateSkeleton(float dt);               // passo di simulazione: se il tavolo è bruciato le braccia cadono
