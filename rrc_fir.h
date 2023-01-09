/*
 * rrc_fir.h
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <complex.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TAU (2.0 * M_PI)

#define NTAPS 127 // lower bauds need more taps, 127 for 300 baud is good
#define GAIN 1.86

void rrc_fir_array(complex double[], complex double[], int);
void rrc_fir(complex double[], complex double *);
void rrc_make(double, double, double);

#ifdef __cplusplus
}
#endif
