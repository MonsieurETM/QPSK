#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <complex.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#define TAU             (2.0 * M_PI)

#define ROTATE_FROM_PLUS_135  (-3.0 * M_PI / 4.0)
#define ROTATE_FROM_PLUS_45   (-1.0 * M_PI / 4.0)
#define ROTATE_FROM_MINUS_45  (1.0 * M_PI / 4.0)
#define ROTATE_FROM_MINUS_135 (3.0 * M_PI / 4.0)

typedef enum {
    D01 = 0b01,
    D00 = 0b00,
    D10 = 0b10,
    D11 = 0b11,
    D99 = 99	// punt
} Dibit;

void create_interpolatingSampleBuffer(double, double);
void interp_receive(complex double);
void resetAndAdjust(double);

double getSamplingPoint(void);

bool hasSymbol(void);

complex double sampleFilter(complex double [], size_t, int, double);
complex double getPrecedingSample(void);
complex double getCurrentSample(void);
complex double getMiddleSample(void);

void create_symbolEvaluator(void);
void setSymbols(complex double, complex double);
double getPhaseError(void);
double getTimingError(void);
Dibit getSymbolDecision(void);

void create_QPSKDemodulator(double, double);
Dibit demod_receive(complex double);

#ifdef __cplusplus
}
#endif

