/*
 * interleave.c
 *
 * Golden Prime Interleaver
 *
 * The number of bits in the frame should be less
 * than the maximum prime value. For example 22 bytes
 * is 176 bits, so you should have a prime number
 * close to that value. Larger frames means larger
 * prime table.
 * 
 * If your frames are always the same size, then you
 * only need one prime number.
 *
 * Reference:
 *
 * "On the Analysis and Design of Good Algebraic
 * Interleavers", Xie et al
 *
 * Make for debug with:
 *    cc -O2 -DDEBUG interleave.c -o interleave -lm
 * or for linking:
 *    cc -O2 -c interleave.c -o interleave.o
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "interleave.h"

static const uint16_t primes[] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
    179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
    233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
    283, 293, 307, 311, 313, 317, 331, 337, 347
};

void interleave(uint8_t *inout, int nbytes, int dir) {
    uint8_t out[nbytes];

    memset(out, 0, nbytes);

    uint16_t imax = sizeof (primes) / sizeof (uint16_t);
    uint16_t nbits = (uint16_t) (nbytes * 8);

    int index = 1;
    while ((primes[index] < nbits) && (index < imax))
        index++;

    uint32_t b = primes[index - 1]; /* b = nearest prime to length of nbits */
    
    for (int n = 0; n < nbits; n++) {
        uint32_t i = n;
        uint32_t j = (b * i) % nbits;

        if (dir == DEINTERLEAVE) {
            uint32_t tmp = j;
            j = i;
            i = tmp;
        }

        uint32_t ibyte = (i / 8);
        uint32_t ishift = (i % 8);
        uint32_t ibit = (inout[ibyte] >> ishift) & 0x1;

        uint32_t jbyte = (j / 8);
        uint32_t jshift = (j % 8);

        out[jbyte] |= (ibit << jshift);
    }

    memcpy(inout, out, nbytes);
}

#ifdef DEBUG
/*
 * I get the following data for 4 bytes interleaved into 8 bytes
 *
 * Original Data:      10101010 10101010 10101010 10101010 00000000 00000000 00000000 00000000 
 * Interleaved Data:   10000010 00100000 00001000 10000010 00101000 10001010 10100010 00101000 
 * Deinterleaved Data: 10101010 10101010 10101010 10101010 00000000 00000000 00000000 00000000
 */
int main(int argc, char **argv) {
    uint8_t data[8] = { 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0, 0, 0, 0 };

    printf("Original Data:      ");

    for (int i = 0; i < 8; i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (data[i] >> j) & 0x1);
        }

        printf(" ");
    }

    printf("\nInterleaved Data:   ");

    interleave(data, 8, INTERLEAVE);

    for (int i = 0; i < 8; i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (data[i] >> j) & 0x1);
        }

        printf(" ");
    }

    printf("\nDeinterleaved Data: ");

    interleave(data, 8, DEINTERLEAVE);

    for (int i = 0; i < 8; i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (data[i] >> j) & 0x1);
        }

        printf(" ");
    }

    printf("\n");
}
#endif
