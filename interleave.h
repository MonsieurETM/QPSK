#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define INTERLEAVE   0
#define DEINTERLEAVE 1

void interleave(uint8_t *, int, int);

#ifdef __cplusplus
}
#endif
