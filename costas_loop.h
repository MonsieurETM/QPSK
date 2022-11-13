#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

void create_costas(float);
void costas(complex float *);
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

float get_loop_bandwidth();
float get_damping_factor();
float get_alpha();
float get_beta();
float get_frequency();
float get_phase();
float get_max_freq(void);
float get_min_freq(void);

#ifdef __cplusplus
}
#endif

