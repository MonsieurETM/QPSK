/*
 * rrc_fir.h
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    
#include <complex.h>

#define NTAPS         127	// lower bauds need more taps, 127 for 300 baud is good
#define GAIN          1.85

void rrc_fir(complex float [], complex float [], int);
void rrc_make(float, float, float);

#ifdef __cplusplus
}
#endif
