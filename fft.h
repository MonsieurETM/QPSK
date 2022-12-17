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
 
#pragma once

#include <complex.h>

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#define TAU             (2.0 * M_PI)

void fft(complex double *, complex double *);
void fftn(complex double *, complex double *, int);
void ifft(complex double *, complex double *);
void ifftn(complex double *, complex double *, int);

