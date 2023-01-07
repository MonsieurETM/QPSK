/*
 * Copyright 2006,2011,2012,2014 Free Software Foundation, Inc.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <complex.h>

void create_costas(double);
void create_control_loop(double, double, double);
void update_gains(void);
void advance_loop(double);
void phase_wrap(void);
void frequency_limit(void);
double phase_detector(complex double);

// Setters

void set_loop_bandwidth(double);
void set_damping_factor(double);
void set_alpha(double);
void set_beta(double);
void set_frequency(double);
void set_phase(double);
void set_max_freq(double);
void set_min_freq(double);
void set_costas_enable(bool);

// Getters

double get_loop_bandwidth(void);
double get_damping_factor(void);
double get_alpha(void);
double get_beta(void);
double get_frequency(void);
double get_phase(void);
double get_max_freq(void);
double get_min_freq(void);
bool get_costas_enable(void);

#ifdef __cplusplus
}
#endif

