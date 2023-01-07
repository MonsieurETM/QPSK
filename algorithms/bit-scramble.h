/*
 * bit-scramble.h
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SEED 0x4A80

/* QPSK 2-bits per symbol */

#define BITS 2

/* Constants  */

typedef enum {
    tx,
    rx,
    both
} SRegister;

/* Prototypes */

void scramble_init(SRegister);
int scramble(uint8_t *, SRegister);

#ifdef __cplusplus
}
#endif

