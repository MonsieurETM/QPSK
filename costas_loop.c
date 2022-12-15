/*
 * Copyright 2006,2011,2012,2014 Free Software Foundation, Inc.
 * Author: Tom Rondeau, 2011
 *
 * Converted to C by Steve Sampson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdbool.h>
#include <complex.h>
#include <math.h>

#include "qpsk.h"
#include "costas_loop.h"

static double d_phase;
static double d_freq;

static double d_max_freq;
static double d_min_freq;

static double d_damping;
static double d_loop_bw;

static double d_alpha;
static double d_beta;

static bool d_enable;

/*
 * A Costas loop carrier recovery algorithm.
 *
 * The Costas loop locks to the center frequency of a signal and
 * downconverts signal to baseband.
 */
void create_control_loop(double loop_bw, double min_freq, double max_freq) {
    set_phase(0.);
    set_frequency(0.);

    set_max_freq(max_freq);
    set_min_freq(min_freq);

    set_damping_factor(sqrtf(2.) / 2.);

    // Calls update_gains() which sets alpha and beta
    set_loop_bandwidth(loop_bw);
    
    set_costas_enable(true);
}

double phase_detector(complex double sample) {
    return ((creal(sample) > 0. ? 1. : -1.) * cimag(sample) -
            (cimag(sample) > 0. ? 1. : -1.) * creal(sample));
}

void update_gains() {
    double denom = ((1. + (2. * d_damping * d_loop_bw)) + (d_loop_bw * d_loop_bw));

    d_alpha = (4. * d_damping * d_loop_bw) / denom;
    d_beta = (4. * d_loop_bw * d_loop_bw) / denom;
}

void advance_loop(double error) {
    d_freq = d_freq + d_beta * error;
    d_phase = d_phase + d_freq + d_alpha * error;
}

void phase_wrap() {
    while (d_phase > TAU)
        d_phase -= TAU;

    while (d_phase < -TAU)
        d_phase += TAU;
}

void frequency_limit() {
    if (d_freq > d_max_freq)
        d_freq = d_max_freq;
    else if (d_freq < d_min_freq)
        d_freq = d_min_freq;
}


// Setters

void set_loop_bandwidth(double bw)
{
    if (bw < 0.) {
        d_loop_bw = 0.;
    }

    d_loop_bw = bw;
    update_gains();
}

void set_damping_factor(double df)
{
    if (df <= 0.) {
        d_damping = 0.;
    }

    d_damping = df;
    update_gains();
}

void set_alpha(double alpha)
{
    if (alpha < 0. || alpha > 1.) {
        d_alpha = 0.;
    }

    d_alpha = alpha;
}

void set_beta(double beta)
{
    if (beta < 0. || beta > 1.) {
        d_beta = 0.;
    }

    d_beta = beta;
}

void set_frequency(double freq)
{
    if (freq > d_max_freq)
        d_freq = d_max_freq;
    else if (freq < d_min_freq)
        d_freq = d_min_freq;
    else
        d_freq = freq;
}

void set_phase(double phase)
{
    d_phase = phase;

    phase_wrap();
}

void set_max_freq(double freq) { d_max_freq = freq; }

void set_min_freq(double freq) { d_min_freq = freq; }

void set_costas_enable(bool val) { d_enable = val; }

// Getters

double get_loop_bandwidth() { return d_loop_bw; }

double get_damping_factor() { return d_damping; }

double get_alpha() { return d_alpha; }

double get_beta() { return d_beta; }

double get_frequency() { return d_freq; }

double get_phase() { return d_phase; }

double get_max_freq() { return d_max_freq; }

double get_min_freq() { return d_min_freq; }

bool get_costas_enable() { return d_enable; }

