/*
 * rrc_fir.c
 */

#include <stdint.h>
#include <math.h>
#include <string.h>

#include "rrc_fir.h"

static double coeffs[NTAPS];

/*
 * FIR Filter with specified impulse length
 */
void rrc_fir(complex double memory[], complex double *sample)
{
    memmove(&memory[0], &memory[1], (NTAPS - 1) * sizeof(complex double));
    memory[(NTAPS - 1)] = *sample;

    complex double y = 0.0;

    for (int i = 0; i < NTAPS; i++)
    {
        y += (memory[i] * coeffs[i]);
    }

    *sample = y * GAIN;
}

/*
 * FIR Filter with specified impulse length
 */
void rrc_fir_array(complex double memory[], complex double sample[], int length)
{
    for (int j = 0; j < length; j++)
    {
        memmove(&memory[0], &memory[1], (NTAPS - 1) * sizeof(complex double));
        memory[(NTAPS - 1)] = sample[j];

        complex double y = 0.0;

        for (int i = 0; i < NTAPS; i++)
        {
            y += (memory[i] * coeffs[i]);
        }

        sample[j] = y * GAIN;
    }
}

void rrc_make(double fs, double rs, double alpha)
{
    double num, den;
    double spb = fs / rs; // samples per bit/symbol

    double scale = 0.;

    for (int i = 0; i < NTAPS; i++)
    {
        double xindx = i - NTAPS / 2;
        double x1 = M_PI * xindx / spb;
        double x2 = 4. * alpha * xindx / spb;
        double x3 = x2 * x2 - 1.;

        if (fabsf(x3) >= 0.000001f)
        { // Avoid Rounding errors...
            if (i != NTAPS / 2)
                num = cosf((1. + alpha) * x1) +
                      sinf((1. - alpha) * x1) / (4. * alpha * xindx / spb);
            else
                num = cosf((1. + alpha) * x1) + (1. - alpha) * M_PI / (4. * alpha);

            den = x3 * M_PI;
        }
        else
        {
            if (alpha == 1.)
            {
                coeffs[i] = -1.;
                scale += coeffs[i];
                continue;
            }

            x3 = (1. - alpha) * x1;
            x2 = (1. + alpha) * x1;

            num = (sinf(x2) * (1. + alpha) * M_PI -
                   cosf(x3) * ((1. - alpha) * M_PI * spb) / (4.f * alpha * xindx) +
                   sinf(x3) * spb * spb / (4. * alpha * xindx * xindx));

            den = -32. * M_PI * alpha * alpha * xindx / spb;
        }

        coeffs[i] = 4. * alpha * num / den;
        scale += coeffs[i];
    }

    for (int i = 0; i < NTAPS; i++)
    {
        coeffs[i] = (coeffs[i] * GAIN) / scale;
    }
}
