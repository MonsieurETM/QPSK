#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <complex.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define TX_FILENAME "/tmp/spectrum.raw"

#define FS 25000.0
#define RS 4800.0
#define CENTER 3500.0

#define CYCLES (int)(FS / RS)

#define FRAME_SIZE 512

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TAU (2.0 * M_PI)

/*
 * This method is much faster than using cexp()
 */
#define cmplx(value) (cos(value) + sin(value) * I)
#define cmplxconj(value) (cos(value) + sin(value) * -I)
#define array_length(a) (sizeof(a) / sizeof((a)[0]))

#ifdef __cplusplus
}
#endif
