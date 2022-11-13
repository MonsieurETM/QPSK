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

#include <complex.h>

void create_costas(float);
void create_control_loop(float, float, float);
void update_gains(void);
void advance_loop(float);
void phase_wrap(void);
void frequency_limit(void);

// Setters

void set_loop_bandwidth(float);
void set_damping_factor(float);
void set_alpha(float);
void set_beta(float);
void set_frequency(float);
void set_phase(float);
void set_max_freq(float);
void set_min_freq(float);

// Getters

float get_loop_bandwidth(void);
float get_damping_factor(void);
float get_alpha(void);
float get_beta(void);
float get_frequency(void);
float get_phase(void);
float get_max_freq(void);
float get_min_freq(void);

#ifdef __cplusplus
}
#endif

