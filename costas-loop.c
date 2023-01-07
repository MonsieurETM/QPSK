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
#include <stdbool.h>

#include "interp.h"
#include "qpsk.h"
#include "costas-loop.h"

static complex double mCurrentVector;

static double mLoopPhase;
static double mLoopFrequency;
static double mAlphaGain;
static double mBetaGain;

static double mMaximumLoopFrequency;
static double mDamping;

static PLLBandwidth mPLLBandwidth;

static void updateLoopBandwidth(void);

/*
 * Costas Loop - phase locked loop designed to automatically synchronize to the
 * incoming carrier frequency in order to zeroize any frequency offset inherent
 * in the signal due to either mis-tuning or carrier frequency drift.
 *
 * @param sampleRate of the incoming samples (25000 in PSK)
 * @param symbolRate of the digital signal (4800 in PSK)
 *
 * Note: Bandwidth set below to PLLBandwidth.BW_200 in PSK
 */
void create_costasLoop(double sampleRate, double symbolRate) {
    mCurrentVector = 0.0;
    mMaximumLoopFrequency = TAU * (symbolRate / 2.0) / sampleRate;
    mPLLBandwidth = BW_200;
    mDamping = sqrt(2.0) / 2.0;
    mLoopFrequency = 0.0;
    mLoopPhase = 0.0;

    updateLoopBandwidth();
}

/*
 * Corrects the current phase tracker when an inverted output is detected.
 * The inversion will take the form of +/- 90 degrees or +/- 180 degrees,
 * although the latter correction is equal regardless of the sign.
 *
 * If the supplied correction value places the loop frequency outside of the
 * max frequency, then the frequency will be corrected 360 degrees in the
 * opposite direction to maintain within the max frequency bounds.
 *
 * @param correction as measured in radians
 */
void correctInversion(double correction) {
    mLoopFrequency += correction;

    while (mLoopFrequency > mMaximumLoopFrequency) {
        mLoopFrequency -= (2.0 * mMaximumLoopFrequency);
    }

    while (mLoopFrequency < -mMaximumLoopFrequency) {
        mLoopFrequency += (2.0 * mMaximumLoopFrequency);
    }
}

/*
 * Updates the loop bandwidth and alpha/beta gains according to the current
 * loop synchronization state.
 */
static void updateLoopBandwidth() {
    double bandwidth = TAU / (double) mPLLBandwidth;

    mAlphaGain = (4.0 * mDamping * bandwidth) / (1.0 + (2.0 * mDamping * bandwidth) + (bandwidth * bandwidth));
    mBetaGain = (4.0 * bandwidth * bandwidth) / (1.0 + (2.0 * mDamping * bandwidth) + (bandwidth * bandwidth));
}

/*
 * Sets the PLLBandwidth state for this costas loop. The bandwidth affects
 * the aggressiveness of the alpha/beta gain values in synchronizing with
 * the signal carrier.
 *
 * @param pllBandwidth set to PLLBandwidth.BW_200 in PSK
 */
void setPLLBandwidth(PLLBandwidth pllBandwidth) {
    if (mPLLBandwidth != pllBandwidth) {
        mPLLBandwidth = pllBandwidth;

        updateLoopBandwidth();
    }
}

/*
 * Increments the phase of the loop for each sample received at the sample
 * rate.
 */
void increment() {
    mLoopPhase += mLoopFrequency;

    /* Normalize phase between +/- TAU */
    if (mLoopPhase > TAU) {
        mLoopPhase -= TAU;
    }

    if (mLoopPhase < -TAU) {
        mLoopPhase += TAU;
    }
}

/*
 * Current vector of the loop.
 *
 * Note: this value is updated for the current angle
 * in radians each time this method is invoked.
 * @return 
 */
complex double getCurrentVector() {
    mCurrentVector = cmplx(mLoopPhase);

    return mCurrentVector;
}

complex double incrementAndGetCurrentVector() {
    increment();

    return getCurrentVector();
}

double getLoopFrequency() {
    return mLoopFrequency;
}

/*
 * Updates the costas loop frequency and phase to adjust for the phase error
 * value
 *
 * @param phaseError - (-)= late and (+)= early
 */
void costas_adjust(double phaseError) {
    mLoopFrequency += (mBetaGain * phaseError);
    mLoopPhase += mLoopFrequency + (mAlphaGain * phaseError);

    // Normalize phase between +/- TAU
    if (mLoopPhase > TAU) {
        mLoopPhase -= TAU;
    }

    if (mLoopPhase < -TAU) {
        mLoopPhase += TAU;
    }

    // Limit frequency to +/- maximum loop frequency
    if (mLoopFrequency > mMaximumLoopFrequency) {
        mLoopFrequency = mMaximumLoopFrequency;
    }

    if (mLoopFrequency < -mMaximumLoopFrequency) {
        mLoopFrequency = -mMaximumLoopFrequency;
    }
}

/*
 * Resets the PLL internal tracking values
 */
void costas_reset() {
    mLoopPhase = 0.0;
    mLoopFrequency = 0.0;
}

