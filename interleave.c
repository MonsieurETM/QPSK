/*
 * interleave.c
 *
 * The number of bits in the frame should be less
 * than the maximum prime value. For example 22 bytes
 * is 176 bits, so you should have a prime number
 * close to that value. Larger frames means larger
 * prime table.
 * 
 * If your frames are always the same size, then you
 * only need one prime number.
 */

#include <stdlib.h>
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
    uint32_t i, j, n, ibit, ibyte, ishift, jbyte, jshift;
    uint32_t b, tmp;
    uint8_t out[nbytes];

    memset(out, 0, nbytes);

    uint16_t imax = sizeof (primes) / sizeof (uint16_t);
    uint16_t nbits = (uint16_t) (nbytes * 8);

    i = 1;
    while ((primes[i] < nbits) && (i < imax))
        i++;

    b = primes[i - 1]; /* b = nearest prime to length of nbits */
    
    for (n = 0; n < nbits; n++) {
        i = n;
        j = (b * i) % nbits;

        if (dir == DEINTERLEAVE) {
            tmp = j;
            j = i;
            i = tmp;
        }

        ibyte = (i / 8);
        ishift = (i % 8);
        ibit = (inout[ibyte] >> ishift) & 0x1;

        jbyte = (j / 8);
        jshift = (j % 8);

        out[jbyte] |= (ibit << jshift);
    }

    memcpy(inout, out, nbytes);
}

