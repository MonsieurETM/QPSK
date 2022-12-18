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

#define FS              9600.0
#define RS              2400.0
#define CENTER          1500.0

#define CYCLES          (int) (FS / RS)

#define FRAME_SIZE      512

#define OS_48           6                   // oversampling rate
#define OS_TAPS_48K     48                  // number of OS filter taps at 48 kHz
#define OS_TAPS_48_8K   (OS_TAPS_48K/OS_48) // number of OS filter taps at 8 kHz

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#define TAU             (2.0 * M_PI)
#define ROTATE45        (M_PI / 4.0)

#define INTERPOLATION   0
#define DECIMATION      1

/*
 * This method is much faster than using cexp()
 */
#define cmplx(value) (cos(value) + sin(value) * I)
#define cmplxconj(value) (cos(value) + sin(value) * -I)
#define array_length(a) (sizeof(a)/sizeof((a)[0]))

#ifdef __cplusplus
}
#endif
