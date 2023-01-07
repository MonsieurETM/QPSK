/*
 * bit-scramble.c
 *
 * Full Duplex Capable, using TX/RX registers
 * 
 * Note: This is not a Cryptographic Scrambler, it is a Bit Scrambler.
 *
 * This Linear Feedback Shift Register is taken from the Digital Video
 * Broadcast System (DVB), which uses the polynomial 1 + X14 + X15.
 *
 * The Sync Seed is reset at the start of each frame transmitted. 
 *
 *          In-Place 15 bit additive scrambler with 0x4a80 Frame Sync
 * 
 *  Sync    1   0   0   1   0   1   0   1   0   0   0   0   0   0   0
 *        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *        | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13| 14| 15|
 *        +-+-+---+---+---+---+---+---+---+---+---+---+---+---+-+-+-+-+
 *          ^                                                   |   |
 *          |                                                   v   v
 *          |                                                 +-------+
 *          +<------------------------------------------------|   +   |
 *          |                                                 +-------+ 
 *          v
 *        +---+
 * in --->| + |---> out
 *        +---+
 *
 * The scrambler is important for modems that don't use NRZI and bit-stuffing
 * as you can insure all carriers and constellation phases get equal time.
 * 
 * Full-Duplex capable
 */

#include <stdint.h>

#include "bit-scramble.h"

// Locals

static uint16_t TXMemory;
static uint16_t RXMemory;

// Functions

void scramble_init(SRegister sr) {
    if (sr == tx) {
        TXMemory = SEED;
    } else if (sr == rx) {
        RXMemory = SEED;
    } else if (sr == both) {
        TXMemory = SEED;
        RXMemory = SEED;
    }
}

static void scramble_internal(uint8_t *input, uint16_t *memory) {
    for (size_t i = 0; i < BITS; i++) {
        uint16_t scrambler_out = (uint16_t) (((*memory & 0x2) >> 1) ^ (*memory & 0x1));
        uint16_t bits = (uint16_t) (((*input >> i) & 0x1) ^ scrambler_out);

        *input &= ~(1 << i);
        *input |= (bits << i);

        /* update scrambler memory */
        *memory >>= 1;
        *memory |= (scrambler_out << 14);
    }
}

/*
 * Returns -1 on error
 */
int scramble(uint8_t *input, SRegister sr) {
    if (sr == tx) {
        scramble_internal(input, &TXMemory);
    } else if (sr == rx) {
        scramble_internal(input, &RXMemory);
    } else if (sr == both) {
        return -1;
    }
    
    return 0;
}
