#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef enum {
    interleave,
    deinterleave
} ILMode;

void interleave(uint8_t *, int, int);

#ifdef __cplusplus
}
#endif

