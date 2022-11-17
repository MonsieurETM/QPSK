#pragma once

#include <stdint.h>

typedef enum {
    interleave,
    deinterleave
} ILMode;

void interleave(uint8_t *, int, int);
