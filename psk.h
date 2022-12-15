/*
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
 */
#pragma once

#include <complex.h>
#include <math.h>

typedef enum {
    MOD_BPSK = 2,
    MOD_QPSK = 4,
    MOD_8QAM = 8,
    MOD_16QAM = 16,
    MOD_32QAM = 32,
    MOD_64QAM = 64
} Mod_type;

void set_predefined_constellation(Mod_type);
void mod(const int *in, int nItems, complex double *out);
void demod(const complex double *in, int nItems, float *out, float variance);
