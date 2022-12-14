/*
 * Original code from:
 *
 * Mercury: A configurable open-source software-defined modem.
 * Copyright (C) 2022 Fadi Jerji
 * Author: Fadi Jerji
 * Email: fadi.jerji@  <gmail.com, rhizomatica.org, caisresearch.com, ieee.org>
 * ORCID: 0000-0002-2076-5831
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Ref:Wickerhauser, Mladen Victor, Mathematics for Multimedia
 * Birkhäuser Boston, January 2010
 * https://www.math.wustl.edu/~victor/mfmm/
 *
 * Converted to C by S. Sampson, December 2022
 */

#include <complex.h>
#include <stdlib.h>
#include <math.h>

#include "fft.h"

// Functions

static void _fft(complex double *v, int n) {
    if (n > 1) {
        complex double tmp[n];
        complex double *ve = &tmp[0];
        complex double *vo = &tmp[n / 2];

        for (int k = 0; k < n / 2; k++) {
            int even = 2 * k;  // calculate once

            ve[k] = v[even];
            vo[k] = v[even + 1];
        }

        _fft(ve, n / 2);
        _fft(vo, n / 2);

        for (int m = 0; m < (n / 2); m++) {
            complex double w = (cos(TAU * (double)m / (double)n)) +
                                   (-sin(TAU * (double)m / (double)n)) * I;
            complex double z = (creal(w) * creal(vo[m]) - cimag(w) * cimag(vo[m])) +
                                   (creal(w) * cimag(vo[m]) + cimag(w) * creal(vo[m])) * I;

            v[m] = (creal(ve[m]) + creal(z)) +
                               (cimag(ve[m]) + cimag(z)) * I;
            v[m + n / 2] = (creal(ve[m]) - creal(z)) +
                               (cimag(ve[m]) - cimag(z)) * I; 
        }
    }
}

static void _ifft(complex double *v, int n) {
    if (n > 1) {
        complex double tmp[n];
        complex double *ve = &tmp[0];
        complex double *vo = &tmp[n / 2];

        for (int k = 0; k < n / 2; k++) {
            int even = 2 * k;  // calculate once

            ve[k] = v[even];
            vo[k] = v[even + 1];
        }

        _ifft(ve, n / 2);
        _ifft(vo, n / 2);

        for (int m = 0; m < (n / 2); m++) {
            complex double w = (cos(TAU * (double)m / (double)n)) +
                                   (sin(TAU * (double)m / (double)n)) * I; // conjugate
            complex double z = (creal(w) * creal(vo[m]) - cimag(w) * cimag(vo[m])) +
                                   (creal(w) * cimag(vo[m]) + cimag(w) * creal(vo[m])) * I;

            v[m] = (creal(ve[m]) + creal(z)) +
                               (cimag(ve[m]) + cimag(z)) * I;
            v[m + n / 2] = (creal(ve[m]) - creal(z)) +
                               (cimag(ve[m]) - cimag(z)) * I; 
        }
    }
}

void fft(complex double *in, complex double *out) {
    for (int i = 0; i < NFFT; i++) {
        out[i] = in[i];
    }

    _fft(out, NFFT);

    for (int i = 0; i < NFFT; i++) {
        out[i] = out[i] / (double)NFFT;
    }
}

void fftn(complex double *in, complex double *out, int n) {
    for (int i = 0; i < n; i++) {
        out[i] = in[i];
    }

    _fft(out, n);

    for (int i = 0; i < n; i++) {
        out[i] = out[i] / (double)n;
    }
}

void ifft(complex double *in, complex double *out) {
    for (int i = 0; i < NFFT; i++) {
        out[i] = in[i];
    }

    _ifft(out, NFFT);
}

void ifftn(complex double *in, complex double *out, int n) {
    for (int i = 0; i < n; i++) {
        out[i] = in[i];
    }

    _ifft(out, n);
}

