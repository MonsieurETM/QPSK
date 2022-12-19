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
 * Converted to C by S. Sampson, December 2022
 */

#include <complex.h>
#include <stdlib.h>
#include <math.h>

#include "psk.h"

static double cnorm(complex double);

complex double *Constellation;

int nBits;
int nSymbols;

void create_psk() {
    Constellation = (complex double *)NULL;
    nBits = 0;
    nSymbols = 0;
}

void destroy_psk() {
    if (Constellation != (complex double *)NULL)
        free(Constellation);
}

static double cnorm(complex double val) {
    double re = creal(val);
    double im = cimag(val);

    return re * re + im * im;
}

void set_constellation(Mod_type M) {
    Constellation = (complex double *) calloc(M, sizeof (complex double));

    if (M == MOD_BPSK) {
        Constellation[0] = CMPLX(1, 0);
        Constellation[1] = CMPLX(-1, 0);
    } else if (M == MOD_QPSK) {
        Constellation[0] = CMPLX(-1, 1);
        Constellation[1] = CMPLX(-1, -1);
        Constellation[2] = CMPLX(1, 1);
        Constellation[3] = CMPLX(1, -1);
    } else if (M == MOD_8QAM) {
        Constellation[0] = CMPLX(-3, 1);
        Constellation[1] = CMPLX(-3, -1);
        Constellation[2] = CMPLX(-1, 1);
        Constellation[3] = CMPLX(-1, -1);
        Constellation[4] = CMPLX(3, 1);
        Constellation[5] = CMPLX(3, -1);
        Constellation[6] = CMPLX(1, 1);
        Constellation[7] = CMPLX(1, -1);
    } else if (M == MOD_16QAM) {
        Constellation[0] = CMPLX(-3, 3);
        Constellation[1] = CMPLX(-3, 1);
        Constellation[2] = CMPLX(-3, -3);
        Constellation[3] = CMPLX(-3, -1);
        Constellation[4] = CMPLX(-1, 3);
        Constellation[5] = CMPLX(-1, 1);
        Constellation[6] = CMPLX(-1, -3);
        Constellation[7] = CMPLX(-1, -1);
        Constellation[8] = CMPLX(3, 3);
        Constellation[9] = CMPLX(3, 1);
        Constellation[10] = CMPLX(3, -3);
        Constellation[11] = CMPLX(3, -1);
        Constellation[12] = CMPLX(1, 3);
        Constellation[13] = CMPLX(1, 1);
        Constellation[14] = CMPLX(1, -3);
        Constellation[15] = CMPLX(1, -1);
    } else if (M == MOD_32QAM) {
        Constellation[0] = CMPLX(-3, 5);
        Constellation[1] = CMPLX(-1, 5);
        Constellation[2] = CMPLX(-3, -5);
        Constellation[3] = CMPLX(-1, -5);
        Constellation[4] = CMPLX(-5, 3);
        Constellation[5] = CMPLX(-5, 1);
        Constellation[6] = CMPLX(-5, -3);
        Constellation[7] = CMPLX(-5, -1);
        Constellation[8] = CMPLX(-1, 3);
        Constellation[9] = CMPLX(-1, 1);
        Constellation[10] = CMPLX(-1, -3);
        Constellation[11] = CMPLX(-1, -1);
        Constellation[12] = CMPLX(-3, 3);
        Constellation[13] = CMPLX(-3, 1);
        Constellation[14] = CMPLX(-3, -3);
        Constellation[15] = CMPLX(-3, -1);
        Constellation[16] = CMPLX(3, 5);
        Constellation[17] = CMPLX(1, 5);
        Constellation[18] = CMPLX(3, -5);
        Constellation[19] = CMPLX(1, -5);
        Constellation[20] = CMPLX(5, 3);
        Constellation[21] = CMPLX(5, 1);
        Constellation[22] = CMPLX(5, -3);
        Constellation[23] = CMPLX(5, -1);
        Constellation[24] = CMPLX(1, 3);
        Constellation[25] = CMPLX(1, 1);
        Constellation[26] = CMPLX(1, -3);
        Constellation[27] = CMPLX(1, -1);
        Constellation[28] = CMPLX(3, 3);
        Constellation[29] = CMPLX(3, 1);
        Constellation[30] = CMPLX(3, -3);
        Constellation[31] = CMPLX(3, -1);
    } else if (M == MOD_64QAM) {
        Constellation[0] = CMPLX(-7, 7);
        Constellation[1] = CMPLX(-7, 5);
        Constellation[2] = CMPLX(-7, 1);
        Constellation[3] = CMPLX(-7, 3);
        Constellation[4] = CMPLX(-7, -7);
        Constellation[5] = CMPLX(-7, -5);
        Constellation[6] = CMPLX(-7, -1);
        Constellation[7] = CMPLX(-7, -3);
        Constellation[8] = CMPLX(-5, 7);
        Constellation[9] = CMPLX(-5, 5);
        Constellation[10] = CMPLX(-5, 1);
        Constellation[11] = CMPLX(-5, 3);
        Constellation[12] = CMPLX(-5, -7);
        Constellation[13] = CMPLX(-5, -5);
        Constellation[14] = CMPLX(-5, -1);
        Constellation[15] = CMPLX(-5, -3);
        Constellation[16] = CMPLX(-1, 7);
        Constellation[17] = CMPLX(-1, 5);
        Constellation[18] = CMPLX(-1, 1);
        Constellation[19] = CMPLX(-1, 3);
        Constellation[20] = CMPLX(-1, -7);
        Constellation[21] = CMPLX(-1, -5);
        Constellation[22] = CMPLX(-1, -1);
        Constellation[23] = CMPLX(-1, -3);
        Constellation[24] = CMPLX(-3, 7);
        Constellation[25] = CMPLX(-3, 5);
        Constellation[26] = CMPLX(-3, 1);
        Constellation[27] = CMPLX(-3, 3);
        Constellation[28] = CMPLX(-3, -7);
        Constellation[29] = CMPLX(-3, -5);
        Constellation[30] = CMPLX(-3, -1);
        Constellation[31] = CMPLX(-3, -3);
        Constellation[32] = CMPLX(7, 7);
        Constellation[33] = CMPLX(7, 5);
        Constellation[34] = CMPLX(7, 1);
        Constellation[35] = CMPLX(7, 3);
        Constellation[36] = CMPLX(7, -7);
        Constellation[37] = CMPLX(7, -5);
        Constellation[38] = CMPLX(7, -1);
        Constellation[39] = CMPLX(7, -3);
        Constellation[40] = CMPLX(5, 7);
        Constellation[41] = CMPLX(5, 5);
        Constellation[42] = CMPLX(5, 1);
        Constellation[43] = CMPLX(5, 3);
        Constellation[44] = CMPLX(5, -7);
        Constellation[45] = CMPLX(5, -5);
        Constellation[46] = CMPLX(5, -1);
        Constellation[47] = CMPLX(5, -3);
        Constellation[48] = CMPLX(1, 7);
        Constellation[49] = CMPLX(1, 5);
        Constellation[50] = CMPLX(1, 1);
        Constellation[51] = CMPLX(1, 3);
        Constellation[52] = CMPLX(1, -7);
        Constellation[53] = CMPLX(1, -5);
        Constellation[54] = CMPLX(1, -1);
        Constellation[55] = CMPLX(1, -3);
        Constellation[56] = CMPLX(3, 7);
        Constellation[57] = CMPLX(3, 5);
        Constellation[58] = CMPLX(3, 1);
        Constellation[59] = CMPLX(3, 3);
        Constellation[60] = CMPLX(3, -7);
        Constellation[61] = CMPLX(3, -5);
        Constellation[62] = CMPLX(3, -1);
        Constellation[63] = CMPLX(3, -3);
    }

    nSymbols = M;
    nBits = (int) log2(M);

    double power_normalization_value = 0.;

    for (int i = 0; i < M; i++) {
        power_normalization_value += cnorm(Constellation[i]);
    }

    power_normalization_value = 1./ (sqrt(power_normalization_value / M));

    for (int i = 0; i < M; i++) {
        Constellation[i] *= power_normalization_value;
    }
}

void psk_mod(const int *in, int nItems, complex double *out) {
    for (int i = 0; i < nItems; i += nBits) {
        unsigned int const_loc = 0U;

        for (int j = 0; j < nBits; j++) {
            const_loc += *(in + i + j);
            const_loc <<= 1;
        }

        const_loc >>= 1;

        *(out + i/nBits) = Constellation[const_loc];
    }
}

void psk_demod(const complex double *in, int nItems, float *out, float variance) {
    float D[nSymbols];
    float LLR[nBits];
    float Dmin0, Dmin1;
    unsigned int mask;

    for (int i = 0; i < nItems; i += nBits) {
        for (int j = 0; j < nSymbols; j++) {
            D[j] = cnorm(*(in + 1 / nBits) - Constellation[j]);
        }

        mask = 1;

        for (int k = 0; k < nBits; k++) {
            Dmin0 = D[0];
            Dmin1 = D[mask];

            for (int j = 0; j < nSymbols; j++) {
                if ((j & mask) == 0) {
                    if(D[j] < Dmin0) {
                        Dmin0 = D[j];
                    }
                }

                if ((j & mask) == mask) {
                    if(D[j] < Dmin1) {
                        Dmin1 = D[j];
                    }
                }
            }

            LLR[k] = ((1. / variance) * (Dmin1 - Dmin0));
            mask <<= 1;
        }

        for (int j = 0; j < nBits; j++) {
            *(out + i + j) = LLR[nBits - j - 1];
        }
    }
}

