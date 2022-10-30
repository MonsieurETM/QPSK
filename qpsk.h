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

#define TX_FILENAME "/tmp/spectrum-filtered.raw"

#define FS              9600.0
#define RS              2400.0
#define NS              8
#define TS              (1.0 / RS)
#define CYCLES          (int) (FS / RS)
#define CENTER          1500.0

#define DATA_SYMBOLS    32

#define DATA_SAMPLES    (DATA_SYMBOLS * CYCLES * NS)
#define FRAME_SIZE      512
#define MAXINDEX        511

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#define TAU             (2.0 * M_PI)

#define ALPHA		0.010
#define BETA		(ALPHA * ALPHA)

/*
 * This method is much faster than using cexp()
 */
#define cmplx(value) (cosf(value) + sinf(value) * I)
#define cmplxconj(value) (cosf(value) + sinf(value) * -I)

typedef struct
{
    int data;
    int tx_symb;
    float cost;
    complex float rx_symb;
} Rxed;

/* Prototypes */

complex float qpsk_mod(int []);
void qpsk_demod(complex float, int []);

int bpsk_pilot_modulate(int16_t []);
int qpsk_data_modulate(int16_t [], int [], int);

int tx_frame(int16_t [], complex float [], int);

void rx_frame(int16_t [], int []);

#ifdef __cplusplus
}
#endif
