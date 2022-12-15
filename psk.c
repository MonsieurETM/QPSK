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

#include "psk.h"

char *copyright_psk = "psk: Copyright (C) 2022 Fadi Jerji";

static void set_constellation(complex double *, Mod_type);
static double cnorm(complex double);

complex double *constellation;

int nBits;
int nSymbols;

void create_psk() {
	constellation = (complex double *)NULL;
	nBits = 0;
	nSymbols = 0;
}

void destroy_psk() {
    if (constellation != (complex double *)NULL)
        free(constellation);
}

void set_predefined_constellation(Mod_type M)
{
	constellation = calloc(M, sizeof (complex double));

	if (M == MOD_BPSK) {
		constellation[0] = CMPLX(1, 0);
		constellation[1] = CMPLX(-1, 0);
	} else if (M == MOD_QPSK) {
		constellation[0] = CMPLX(-1, 1);
		constellation[1] = CMPLX(-1, -1);
		constellation[2] = CMPLX(1, 1);
		constellation[3] = CMPLX(1, -1);
	} else if (M == MOD_8QAM) {
		constellation[0] = CMPLX(-3, 1);
		constellation[1] = CMPLX(-3, -1);
		constellation[2] = CMPLX(-1, 1);
		constellation[3] = CMPLX(-1, -1);
		constellation[4] = CMPLX(3, 1);
		constellation[5] = CMPLX(3, -1);
		constellation[6] = CMPLX(1, 1);
		constellation[7] = CMPLX(1, -1);
	} else if (M == MOD_16QAM) {
		constellation[0] = CMPLX(-3, 3);
		constellation[1] = CMPLX(-3, 1);
		constellation[2] = CMPLX(-3, -3);
		constellation[3] = CMPLX(-3, -1);
		constellation[4] = CMPLX(-1, 3);
		constellation[5] = CMPLX(-1, 1);
		constellation[6] = CMPLX(-1, -3);
		constellation[7] = CMPLX(-1, -1);
		constellation[8] = CMPLX(3, 3);
		constellation[9] = CMPLX(3, 1);
		constellation[10] = CMPLX(3, -3);
		constellation[11] = CMPLX(3, -1);
		constellation[12] = CMPLX(1, 3);
		constellation[13] = CMPLX(1, 1);
		constellation[14] = CMPLX(1, -3);
		constellation[15] = CMPLX(1, -1);
	} else if (M == MOD_32QAM) {
		constellation[0] = CMPLX(-3, 5);
		constellation[1] = CMPLX(-1, 5);
		constellation[2] = CMPLX(-3, -5);
		constellation[3] = CMPLX(-1, -5);
		constellation[4] = CMPLX(-5, 3);
		constellation[5] = CMPLX(-5, 1);
		constellation[6] = CMPLX(-5, -3);
		constellation[7] = CMPLX(-5, -1);
		constellation[8] = CMPLX(-1, 3);
		constellation[9] = CMPLX(-1, 1);
		constellation[10] = CMPLX(-1, -3);
		constellation[11] = CMPLX(-1, -1);
		constellation[12] = CMPLX(-3, 3);
		constellation[13] = CMPLX(-3, 1);
		constellation[14] = CMPLX(-3, -3);
		constellation[15] = CMPLX(-3, -1);
		constellation[16] = CMPLX(3, 5);
		constellation[17] = CMPLX(1, 5);
		constellation[18] = CMPLX(3, -5);
		constellation[19] = CMPLX(1, -5);
		constellation[20] = CMPLX(5, 3);
		constellation[21] = CMPLX(5, 1);
		constellation[22] = CMPLX(5, -3);
		constellation[23] = CMPLX(5, -1);
		constellation[24] = CMPLX(1, 3);
		constellation[25] = CMPLX(1, 1);
		constellation[26] = CMPLX(1, -3);
		constellation[27] = CMPLX(1, -1);
		constellation[28] = CMPLX(3, 3);
		constellation[29] = CMPLX(3, 1);
		constellation[30] = CMPLX(3, -3);
		constellation[31] = CMPLX(3, -1);
	} else if (M == MOD_64QAM) {
		constellation[0] = CMPLX(-7, 7);
		constellation[1] = CMPLX(-7, 5);
		constellation[2] = CMPLX(-7, 1);
		constellation[3] = CMPLX(-7, 3);
		constellation[4] = CMPLX(-7, -7);
		constellation[5] = CMPLX(-7, -5);
		constellation[6] = CMPLX(-7, -1);
		constellation[7] = CMPLX(-7, -3);
		constellation[8] = CMPLX(-5, 7);
		constellation[9] = CMPLX(-5, 5);
		constellation[10] = CMPLX(-5, 1);
		constellation[11] = CMPLX(-5, 3);
		constellation[12] = CMPLX(-5, -7);
		constellation[13] = CMPLX(-5, -5);
		constellation[14] = CMPLX(-5, -1);
		constellation[15] = CMPLX(-5, -3);
		constellation[16] = CMPLX(-1, 7);
		constellation[17] = CMPLX(-1, 5);
		constellation[18] = CMPLX(-1, 1);
		constellation[19] = CMPLX(-1, 3);
		constellation[20] = CMPLX(-1, -7);
		constellation[21] = CMPLX(-1, -5);
		constellation[22] = CMPLX(-1, -1);
		constellation[23] = CMPLX(-1, -3);
		constellation[24] = CMPLX(-3, 7);
		constellation[25] = CMPLX(-3, 5);
		constellation[26] = CMPLX(-3, 1);
		constellation[27] = CMPLX(-3, 3);
		constellation[28] = CMPLX(-3, -7);
		constellation[29] = CMPLX(-3, -5);
		constellation[30] = CMPLX(-3, -1);
		constellation[31] = CMPLX(-3, -3);
		constellation[32] = CMPLX(7, 7);
		constellation[33] = CMPLX(7, 5);
		constellation[34] = CMPLX(7, 1);
		constellation[35] = CMPLX(7, 3);
		constellation[36] = CMPLX(7, -7);
		constellation[37] = CMPLX(7, -5);
		constellation[38] = CMPLX(7, -1);
		constellation[39] = CMPLX(7, -3);
		constellation[40] = CMPLX(5, 7);
		constellation[41] = CMPLX(5, 5);
		constellation[42] = CMPLX(5, 1);
		constellation[43] = CMPLX(5, 3);
		constellation[44] = CMPLX(5, -7);
		constellation[45] = CMPLX(5, -5);
		constellation[46] = CMPLX(5, -1);
		constellation[47] = CMPLX(5, -3);
		constellation[48] = CMPLX(1, 7);
		constellation[49] = CMPLX(1, 5);
		constellation[50] = CMPLX(1, 1);
		constellation[51] = CMPLX(1, 3);
		constellation[52] = CMPLX(1, -7);
		constellation[53] = CMPLX(1, -5);
		constellation[54] = CMPLX(1, -1);
		constellation[55] = CMPLX(1, -3);
		constellation[56] = CMPLX(3, 7);
		constellation[57] = CMPLX(3, 5);
		constellation[58] = CMPLX(3, 1);
		constellation[59] = CMPLX(3, 3);
		constellation[60] = CMPLX(3, -7);
		constellation[61] = CMPLX(3, -5);
		constellation[62] = CMPLX(3, -1);
		constellation[63] = CMPLX(3, -3);
	}

	set_constellation(constellation, M);
}

static double cnorm(complex double val) {
    double re = creal(val);
    double im = cimag(val);

    return re * re + im * im;
}

static void set_constellation(complex double *_constellation, Mod_type size) {
	float power_normalization_value = 0.0f;

	constellation = calloc(size, sizeof (complex double));
	nSymbols = size;

	nBits = (int) log2(nSymbols);

	for (int i = 0; i < size; i++)	{
		constellation[i] = *(_constellation + i);
	}

	for (int i = 0; i < nSymbols; i++) {
		power_normalization_value += cnorm(constellation[i]);
	}

	power_normalization_value = 1./ (sqrt(power_normalization_value / nSymbols));

	for (int i = 0; i < nSymbols; i++) {
		constellation[i] *= power_normalization_value;
	}
}

void psk_mod(const int *in, int nItems, complex double *out) {
	for (int i = 0; i < nItems; i += nBits)	{
		unsigned int const_loc = 0;
		
		for (int j = 0; j < nBits; j++) {
			const_loc += *(in + i + j);
			const_loc <<= 1;
		}

		const_loc >>= 1;

		*(out + i/nBits) = constellation[const_loc];
	}
}

void psk_demod(const complex double *in, int nItems, float *out, float variance) {
	float D[nSymbols];
	float LLR[nBits];
	float Dmin0, Dmin1;
	unsigned int mask;

	for (int i = 0; i < nItems; i += nBits) {
		for (int j = 0; j < nSymbols; j++) {
			D[j] = pow((creal(*(in+i/nBits)) - creal(constellation[j])),2) + pow((cimag(*(in+i/nBits)) - cimag(constellation[j])), 2);
		}

		mask=1;

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

