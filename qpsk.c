/*
 * qpsk.c
 *
 * Testing program for qpsk modem algorithms, November 2022
 */

// Includes

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <complex.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "qpsk.h"
#include "rrc_fir.h"
#include "interp.h"
#include "costas-loop.h"

// Prototypes

static void rx_frame(int16_t *, int []);
static complex double qpsk_mod(int []);
static int tx_frame(int16_t [], complex double [], int);
static complex double qpsk_mod(int []);
static int qpsk_packet_mod(int16_t [], int [], int);

// Globals

FILE *fin;
FILE *fout;

complex double tx_filter[NTAPS];
complex double rx_filter[NTAPS];

// Two phase for full duplex

complex double fbb_tx_phase;
complex double fbb_tx_rect;

complex double fbb_rx_phase;
complex double fbb_rx_rect;

double fbb_offset_freq;

/*
 * QPSK Quadrant bit-pair values - Gray Coded
 */
static const complex double constellation[] = {
    1.0 + 0.0 * I, //  I
    0.0 + 1.0 * I, //  Q
    0.0 - 1.0 * I, // -Q
    -1.0 + 0.0 * I // -I
};

/*
 * Receive function
 * 
 * 2400 baud QPSK at 9600 samples/sec.
 *
 * Remove any frequency and timing offsets
 */
static void rx_frame(int16_t *in, int bits[]) {
    /*
     * Convert input PCM to complex samples
     * at 9600 Hz sample rate to baseband
     */
    fbb_rx_phase *= fbb_rx_rect;

    complex double sample = fbb_rx_phase * ((double) *in / 16384.0f);

    /*
     * Raised Root Cosine Filter
     */
    rrc_fir(rx_filter, &sample);

    Dibit dbit = demod_receive(sample);

    if (dbit != D99) {
#ifdef TEST_SCATTER
        fprintf(stderr, "%f %f\n", creal(sample), cimag(sample));
#endif
    }

    /*
     * Save the detected frequency error
     */
    fbb_offset_freq = (getLoopFrequency() * RS / TAU);	// convert radians to freq at symbol rate
}

/*
 * Modulate the symbols by first upsampling to 9600 Hz sample rate,
 * and translating the spectrum to 1500 Hz, where it is filtered
 * using the root raised cosine coefficients.
 */
static int tx_frame(int16_t samples[], complex double symbol[], int length) {
    complex double signal[(length * CYCLES)];

    /*
     * Build the 2400 baud packet Frame zero padding
     * for the desired 9600 Hz sample rate.
     */
    for (int i = 0; i < length; i++) {
        signal[(i * CYCLES)] = symbol[i];

        for (int j = 1; j < CYCLES; j++) {
            signal[(i * CYCLES) + j] = 0.0;
        }
    }

    /*
     * Raised Root Cosine Filter
     */
    rrc_fir_array(tx_filter, signal, (length * CYCLES));

    /*
     * Shift Baseband to Center Frequency
     */
    for (int i = 0; i < (length * CYCLES); i++) {
        fbb_tx_phase *= fbb_tx_rect;
        signal[i] *= fbb_tx_phase;
    }

    fbb_tx_phase /= cabsf(fbb_tx_phase); // normalize as magnitude can drift

    /*
     * Now return the resulting real samples
     * (imaginary part discarded)
     */
    for (int i = 0; i < (length * CYCLES); i++) {
        samples[i] = (int16_t) (creal(signal[i]) * 16384.0); // I at @ .5
    }

    return (length * CYCLES);
}

/*
 * Gray coded QPSK modulation function
 */
static complex double qpsk_mod(int bits[]) {
    return constellation[(bits[1] << 1) | bits[0]];
}

static int qpsk_packet_mod(int16_t samples[], int tx_bits[], int length) {
    complex double symbol[length];
    int dibit[2];

    for (int i = 0, s = 0; i < length; i++, s += 2) {
        dibit[0] = tx_bits[s + 1] & 0x1;
        dibit[1] = tx_bits[s ] & 0x1;

        symbol[i] = qpsk_mod(dibit);
    }

    return tx_frame(samples, symbol, length);
}

// Main Program

int main(int argc, char** argv) {
    int bits[6400];
    int16_t frame[1];
    int length;

    srand(time(0));

    create_costasLoop(FS, RS);
    double samplesPerSymbol = FS / RS;

    create_QPSKDemodulator(samplesPerSymbol, 0.1);

    /*
     * Create an RRC filter using the
     * Sample Rate, baud, and Alpha
     */
    rrc_make(FS, RS, .35);

    /*
     * create the QPSK data waveform.
     * This simulates the transmitted packets.
     */
    fout = fopen(TX_FILENAME, "wb");

    fbb_tx_phase = cmplx(0.0);
    //fbb_tx_rect = cmplx(TAU * CENTER / FS);
    //fbb_offset_freq = CENTER;

    fbb_tx_rect = cmplx(TAU * (CENTER + 50.0) / FS);
    fbb_offset_freq = (CENTER + 50.0);

    for (int k = 0; k < 100; k++) {
        // 256 QPSK
        for (int i = 0; i < FRAME_SIZE; i++) {
            bits[i] = rand() % 2;
        }

        length = qpsk_packet_mod(frame, bits, (FRAME_SIZE / 2));

        fwrite(frame, sizeof (int16_t), length, fout);
    }

    fclose(fout);

    /*
     * Now try to process what was transmitted
     */
    fin = fopen(TX_FILENAME, "rb");

    fbb_rx_phase = cmplx(0.0);
    fbb_rx_rect = cmplxconj(TAU * CENTER / FS);

    while (1) {
        /*
         * Read in the frame samples
         */
        size_t count = fread(frame, sizeof (int16_t), 1, fin);

        if (count != 1)
            break;

        rx_frame(frame, bits);
    }
    
    fclose(fin);

    return (EXIT_SUCCESS);
}
