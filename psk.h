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
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <complex.h>

typedef enum {
    MOD_BPSK = 2,
    MOD_QPSK = 4,
    MOD_8QAM = 8,
    MOD_16QAM = 16,
    MOD_32QAM = 32,
    MOD_64QAM = 64
} Mod_type;

void create_psk(void);
void destroy_psk(void);
void set_constellation(Mod_type);
void psk_mod(const int *, int, complex double *);
void psk_demod(const complex double *, int, double *, double);

#ifdef __cplusplus
}
#endif

