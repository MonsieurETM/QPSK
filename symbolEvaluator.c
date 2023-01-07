/*
 * Copyright (C) 2015 Dennis Sheirer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Translated from Java S. Sampson
 */

#include <stdio.h>
#include <math.h>
#include <complex.h>

#include "qpsk.h"
#include "interp.h"

static double mPhaseError;
static double mTimingError;

static Dibit mSymbolDecision;

static complex double mPreviousSymbol;
static complex double mEvaluationSymbol;

/*
 * Differential QPSK Decision-directed symbol phase and timing error
 * detector and symbol decision slicer.
 *
 * Symbol decision is based on the closest reference quadrant for the
 * sampled symbol.
 *
 * Phase error is calculated as the angular distance of the sampled symbol
 * from the reference symbol.
 *
 * Timing error is calculated using the Gardner method by comparing the
 * previous symbol to the current symbol and amplifying the delta between
 * the two using the intra-symbol sample to form the timing error.
 */
void create_symbolEvaluator() {
    mPhaseError = 0.0;
    mTimingError = 0.0;

    mSymbolDecision = D00;

    mPreviousSymbol = 0.0;
    mEvaluationSymbol = 0.0;
}

/*
 * Constrains timing error to +/- the maximum value and corrects any
 * floating point invalid numbers
 */
static double enormalize(double error, double maximum) {
    if (isnan(error)) {
        return 0.0;
    }

    // clip - Constrains value to the range of ( -maximum <> maximum )

    if (error > maximum) {
        return maximum;
    } else if (error < -maximum) {
        return -maximum;
    }

    return error;
}

/*
 * Sets the middle and current symbols to be evaluated for phase and timing
 * errors and to determine the transmitted symbol relative to the closest
 * reference symbol. After invoking this method, you can access the phase
 * and timing errors and the symbol decision via their respective accessor
 * methods.
 *
 * Phase and timing error values are calculated by first determining the
 * symbol and then calculating the phase and timing errors relative to the
 * reference symbol. The timing error is corrected with the appropriate sign
 * relative to the angular vector rotation so that the error value indicates
 * the correct error direction.
 *
 * @param middle interpolated differentially-decoded sample that falls
 * midway between previous/current symbols
 * @param current interpolated differentially-decoded symbol
 */
void setSymbols(complex double middle, complex double current) {
    // Gardner timing error calculation
    double errorInphase = (creal(mPreviousSymbol) - creal(current)) * creal(middle);
    double errorQuadrature = (cimag(mPreviousSymbol) - cimag(current)) * cimag(middle);

    mTimingError = enormalize(errorInphase + errorQuadrature, 0.3);

    // Store the current symbol to use in the next symbol calculation
    mPreviousSymbol = current;

    // Phase error and symbol decision calculations ...
    mEvaluationSymbol = current;

    if (cimag(mEvaluationSymbol) > 0.0) {
        if (creal(mEvaluationSymbol) > 0.0) {
            mSymbolDecision = D00;
            mEvaluationSymbol *= cmplx(ROTATE_FROM_PLUS_45);
        } else {
            mSymbolDecision = D01;
            mEvaluationSymbol *= cmplx(ROTATE_FROM_PLUS_135);
        }
    } else {
        if (creal(mEvaluationSymbol) > 0.0) {
            mSymbolDecision = D10;
            mEvaluationSymbol *= cmplx(ROTATE_FROM_MINUS_45);
        } else {
            mSymbolDecision = D11;
            mEvaluationSymbol *= cmplx(ROTATE_FROM_MINUS_135);
        }
    }

    /*
     * Since we've rotated the error symbol back to 0 radians,
     * the Imaginary value closely approximates the arctan of
     * the error angle, relative to 0 radians, and this
     * provides our error value
     */
    mPhaseError = enormalize(-cimag(mEvaluationSymbol), 0.3);
}

/*
 * Phase error of the symbol relative to the nearest reference symbol.
 *
 * @return phase error in radians of distance from the reference symbol.
 */
double getPhaseError() {
    return mPhaseError;
}

/*
 * Reference symbol that is closest to the transmitted/sampled symbol.
 * @return 
 */
Dibit getSymbolDecision() {
    return mSymbolDecision;
} 

/*
 * Timing error of the symbol relative to the nearest reference symbol.
 *
 * @return timing error in radians of angular distance from the reference
 * symbol recognizing that the symbol originates at zero radians and rotates
 * toward the intended reference symbol, therefore the error value indicates
 * if the symbol was sampled early (-) or late (+) relative to the reference
 * symbol.
 */
double getTimingError() {
    return mTimingError;
}

