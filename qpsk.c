/*
 * qpsk.c
 *
 * Testing program for qpsk modem algorithms, December 2022
 *
 * Designed for 2400 Baud, 4800 bit/s
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

#include "psk.h"
#include "qpsk.h"
#include "costas_loop.h"
#include "rrc_fir.h"

// Externs

extern complex double *constellation;

extern int Nfft;
extern int nBits;
extern int nSymbols;

// Prototypes

static void rx_frame(int16_t [], float []);
static int tx_frame(int16_t [], complex double [], int);

// Globals

FILE *fin;
FILE *fout;

complex double tx_filter[NTAPS];
complex double rx_filter[NTAPS];

complex double input_frame[FRAME_SIZE];
complex double decimated_frame[FRAME_SIZE / 2];
complex double costas_frame[FRAME_SIZE / CYCLES];

// Two phases for full duplex

complex double fbb_tx_phase;
complex double fbb_tx_rect;

complex double fbb_rx_phase;
complex double fbb_rx_rect;

double fbb_offset_freq;

double d_error;

/*
 * Receive function
 * 
 * 2400 baud/4800 bit/s QPSK at 9600 samples/sec.
 *
 * Remove any frequency and timing offsets
 */
static void rx_frame(int16_t in[], float output[]) {
    /*
     * Convert input PCM to complex samples
     * at 9600 Hz sample rate
     */
    for (int i = 0; i < FRAME_SIZE; i++) {
        fbb_rx_phase *= fbb_rx_rect;

        input_frame[i] = fbb_rx_phase * ((double) in[i] / 16384.0f);
    }

    fbb_rx_phase /= cabs(fbb_rx_phase); // normalize as magnitude can drift

    /*
     * Raised Root Cosine Filter at baseband
     */
    rrc_fir(rx_filter, input_frame, FRAME_SIZE);

    /*
     * Decimate by 4 to the 2400 symbol rate
     */
    for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
        int extended = (FRAME_SIZE / CYCLES) + i; // compute once
        
        decimated_frame[i] = decimated_frame[extended];		// use previous frame
        decimated_frame[extended] = input_frame[(i * CYCLES) + 6];	// current frame
    }

    if (get_costas_enable() == true) {
        /*
         * Costas Loop over the decimated frame
         */
        for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
            costas_frame[i] = decimated_frame[i] * cmplxconj(get_phase());

#ifdef TEST_SCATTER
            fprintf(stderr, "%f %f\n", creal(costas_frame[i]), cimag(costas_frame[i]));
#endif

            d_error = phase_detector(costas_frame[i]);

            advance_loop(d_error);
            phase_wrap();
            frequency_limit();
        }

        psk_demod(costas_frame, (FRAME_SIZE / CYCLES), output, 2.6f);
    } else {
        for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
#ifdef TEST_SCATTER
            fprintf(stderr, "%f %f\n", creal(decimated_frame[i]), cimag(decimated_frame[i]));
#endif
        }

        psk_demod(decimated_frame, (FRAME_SIZE / CYCLES), output, 2.6f);
    }

    /*
     * Save the detected frequency error
     */
    fbb_offset_freq = (get_frequency() * RS / TAU);	// convert radians to freq at symbol rate
}

/*
 * Modulate the 2400 sym/s. First re-sample at 9600 samples/s inserting zero's,
 * then post filtered using the root raised cosine FIR, then translated to
 * 1500 Hz center frequency.
 */
static int tx_frame(int16_t samples[], complex double symbol[], int length) {
    int n = (length * CYCLES);
    complex double signal[n];	// big enough for 4x sample rate

    /*
     * Build the 2400 sym/s packet Frame by zero padding
     * for the desired (4x nyquist) 9600 sample rate.
     *
     * signal[] array length is now 4x longer.
     */
    for (int i = 0; i < length; i++) {
        int index = (i * CYCLES);	// compute once

        signal[index] = symbol[i];

        for (int j = 1; j < CYCLES; j++) {
            signal[index + j] = 0.;
        }
    }

    /*
     * Raised Root Cosine Filter at Baseband and 9600 samples/s.
     * This optimizes digital data signals
     */
    rrc_fir(tx_filter, signal, n);

    /*
     * Shift Baseband to Center Frequency
     */
    for (int i = 0; i < n; i++) {
        fbb_tx_phase *= fbb_tx_rect;
        signal[i] *= fbb_tx_phase;
    }

    fbb_tx_phase /= cabs(fbb_tx_phase); // normalize as magnitude can drift

    /*
     * Now return the resulting real samples
     * (imaginary part discarded for radio transmit)
     */
    for (int i = 0; i < n; i++) {
        samples[i] = (int16_t) (creal(signal[i]) * 16384.); // I at @ .5
    }

    return n;
}

// Main Program

int main(int argc, char** argv) {
    complex double qpskout[FRAME_SIZE];
    int bits[6400];
    int16_t frame[FRAME_SIZE];
    int length;

    srand(time(0));

    Nfft = 512;

    /*
     * All terms are radians per sample.
     *
     * The loop bandwidth determins the lock range
     * and should be set around TAU/100 to TAU/200
     */
    create_control_loop((TAU / 200.), -1., 1.);

    set_costas_enable(true);

    /*
     * Create an RRC filter using the
     * Sample Rate, baud, and Alpha
     */
    rrc_make(FS, RS, .35);

    /*
     * Create QPSK functions
     */
    create_psk();
    set_predefined_constellation(MOD_QPSK);

    /*
     * create the QPSK data waveform.
     * This simulates the transmitted packets.
     */
    fout = fopen(TX_FILENAME, "wb");

    fbb_tx_phase = cmplx(0.);
    //fbb_tx_rect = cmplx(TAU * CENTER / FS);
    //fbb_offset_freq = CENTER;

    fbb_tx_rect = cmplx(TAU * (CENTER + 50.) / FS);	// 50 Hz Frequency Error
    fbb_offset_freq = (CENTER + 50.);

    for (int k = 0; k < 100; k++) {

        // 256 QPSK dibits of just random bits

        for (int i = 0; i < FRAME_SIZE; i++) {
            bits[i] = rand() % 2;
        }

        psk_mod(bits, FRAME_SIZE, qpskout);
        
        // FRAME_SIZE / 2 is number of QPSK symbols
        length = tx_frame(frame, qpskout, FRAME_SIZE/2);

        fwrite(frame, sizeof (int16_t), length, fout);
    }

    fclose(fout);

    /*
     * Now try to process what was transmitted
     */
    fin = fopen(TX_FILENAME, "rb");

    fbb_rx_phase = cmplx(0.);
    fbb_rx_rect = cmplxconj(TAU * CENTER / FS);

    float output[(FRAME_SIZE / CYCLES)];

    while (1) {
        /*
         * Read in the frame samples
         */
        size_t count = fread(frame, sizeof (int16_t), FRAME_SIZE, fin);

        if (count != FRAME_SIZE)
            break;

        rx_frame(frame, output);
    }
    
    fclose(fin);

    destroy_psk();

    return (EXIT_SUCCESS);
}

