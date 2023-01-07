#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    BW_400 = 400,
    BW_300 = 300,
    BW_250 = 250,
    BW_200 = 200
} PLLBandwidth;

void create_costasLoop(double, double);
void correctInversion(double);
void setPLLBandwidth(PLLBandwidth);
void increment(void);

complex double getCurrentVector(void);
complex double incrementAndGetCurrentVector(void);

double getLoopFrequency(void);

void costas_adjust(double);
void costas_reset(void);

#ifdef __cplusplus
}
#endif

