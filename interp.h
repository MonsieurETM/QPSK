#pragma once

#include <complex.h>
#include <stdbool.h>

void create_interpolatingSampleBuffer(double, double);
void receive(complex double);
void resetAndAdjust(double);

double getSamplingPoint(void);

bool hasSymbol(void);

complex double sampleFilter(complex double [], int, double);
complex double getPrecedingSample(void);
complex double getCurrentSample(void);
complex double getMiddleSample(void);

