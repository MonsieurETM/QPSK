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
 * Translated from Java by S. Sampson
 */

#include <stdlib.h>
#include <complex.h>
#include <math.h>

#include "qpsk.h"
#include "interp.h"
#include "costas-loop.h"

static complex double mPreviousCurrentSample;
static complex double mPreviousMiddleSample;
static complex double mMiddleSymbol;
static complex double mCurrentSymbol;
static complex double mReceivedSample;

/*
 * Implements a Differential QPSK demodulator using a Costas Loop and
 * a Gardner timing error detector.
 */
void create_QPSKDemodulator(double samplesPerSymbol, double sampleCounterGain) {
    create_symbolEvaluator();
    create_interpolatingSampleBuffer(samplesPerSymbol, sampleCounterGain);

    mPreviousCurrentSample = 0.0;
    mPreviousMiddleSample = 0.0;
    mMiddleSymbol = 0.0;
    mCurrentSymbol = 0.0;
    mReceivedSample = 0.0;
}

static complex double cnormalize(complex double a) {
    double mag = cabs(a);

    if (mag != 0.0) {
        return a / mag;
    } else {
        return a;
    }
}

/*
 * Note: the interpolating sample buffer holds 2 symbols worth of samples
 * and the current sample method points to the sample at the mid-point
 * between those 2 symbol periods and the middle sample method points to the
 * sample that is half a symbol period after the current sample.
 *
 * Since we need a middle sample and a current symbol sample for the gardner
 * calculation, we'll treat the interpolating buffer's current sample as the
 * gardner mid-point and we'll treat the interpolating buffer's mid-point
 * sample as the current symbol sample (ie flip-flopped)
 */
static Dibit calculateSymbol() {
    complex double middleSample = getCurrentSample();
    complex double currentSample = getMiddleSample();

    if ((middleSample == -10000.0) || (currentSample == -10000.0)) {
        return D99;
    }

    // Differential decode middle and current symbols by calculating the angular rotation between the previous and
    // current samples (current sample x complex conjugate of previous sample).
    mMiddleSymbol = middleSample * conj(mPreviousMiddleSample);
    mCurrentSymbol = currentSample * conj(mPreviousCurrentSample);

    // Set gain to unity before we calculate the error value
    mMiddleSymbol = cnormalize(mMiddleSymbol);
    mCurrentSymbol = cnormalize(mCurrentSymbol);

    // Pass symbols to evaluator to determine timing and phase error and make symbol decision
    setSymbols(mMiddleSymbol, mCurrentSymbol);

    // Update symbol timing error in Interpolator
    resetAndAdjust(getTimingError());

    // Update Costas phase error from symbolEvaluator
    double d_error = phase_detector(mReceivedSample);

    advance_loop(d_error);
    phase_wrap();
    frequency_limit();

    // Store current samples/symbols for next symbol calculation
    mPreviousMiddleSample = middleSample;
    mPreviousCurrentSample = currentSample;

    return getSymbolDecision();
}

/*
 * Processes a complex sample for decoding. Once sufficient samples are
 * buffered, a symbol decision is made.
 */
Dibit demod_receive(complex double sample) {
    // Update current sample and Mix with costas loop
    // to remove any rotation from frequency error
    mReceivedSample = sample * cmplxconj(get_phase());

    // Store the sample in the interpolating buffer
    interp_receive(mReceivedSample);

    // Calculate the symbol once we've stored enough samples
    if (hasSymbol()) {
        return calculateSymbol();
    }

    return D99;
}

complex double getReceivedSample() {
    return mReceivedSample;
}

