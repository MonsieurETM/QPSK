/*
 * qpsk.c
 *
 * Testing program for qpsk modem algorithms, December 2022
 *
 * Designed for 1200 Baud on 10 meters, 2400 bit/s
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
#include "costas_loop.h"
#include "rrc_fir.h"

// Prototypes

static void qpsk_demod(complex float, int []);
static void rx_frame(int16_t [], int []);
static complex float qpsk_mod(int []);
static int tx_frame(int16_t [], complex float [], int);
static complex float qpsk_mod(int []);
static int qpsk_packet_mod(int16_t [], int [], int);

// Globals

FILE *fin;
FILE *fout;

complex float tx_filter[NTAPS];
complex float rx_filter[NTAPS];

complex float input_frame[FRAME_SIZE];
complex float decimated_frame[FRAME_SIZE / 4];
complex float costas_frame[FRAME_SIZE / CYCLES];

// Two phase for full duplex

complex float fbb_tx_phase;
complex float fbb_tx_rect;

complex float fbb_rx_phase;
complex float fbb_rx_rect;

float fbb_offset_freq;

float d_error;

/*
 * QPSK Quadrant bit-pair values - Gray Coded
 */
const complex float constellation[] = {
    1.0f + 0.0f * I, //  I
    0.0f + 1.0f * I, //  Q
    0.0f - 1.0f * I, // -Q
    -1.0f + 0.0f * I // -I
};

/*
 * Gray coded QPSK demodulation function
 *
 * By rotating received symbol 45 degrees the bits
 * are easier to decode as they are in a specific
 * rectangular quadrants.
 * 
 * Each bit pair differs from the next by only one bit.
 */
static void qpsk_demod(complex float symbol, int bits[]) {
    /*
     * Don't rotate when costas loop enabled
     */
    if (get_costas_enable() == false) {
        symbol *= cmplx(ROTATE45);
    }

    bits[0] = crealf(symbol) < 0.0f; // I < 0 ?
    bits[1] = cimagf(symbol) < 0.0f; // Q < 0 ?
}

/*
 * Receive function
 * 
 * 1200 baud QPSK at 9600 sample rate
 *
 * Remove any frequency and timing offsets
 */
static void rx_frame(int16_t in[], int bits[]) {
    /*
     * You need as many histograms as you think
     * you'll need for timing offset. Using 8 for now.
     * First [0] index not used.
     */
    int hist_i[8] = { 0 };
    int hist_q[8] = { 0 };
    int hist[8] = { 0 };

    /*
     * Convert input PCM to complex samples at 9600 sample rate
     * by translating from 1500 Hz center frequency to baseband.
     */
    for (int i = 0; i < FRAME_SIZE; i++) {
        fbb_rx_phase *= fbb_rx_rect;

        input_frame[i] = fbb_rx_phase * ((float) in[i] / 16384.0f);	// +/- .5 or so
    }

    fbb_rx_phase /= cabsf(fbb_rx_phase); // normalize as magnitude can drift

    /*
     * Raised Root Cosine Baseband Filter
     */
    rrc_fir(rx_filter, input_frame, FRAME_SIZE);	// 1200 Baud, 9600 sample rate

    float max_i = 0.0f;
    float max_q = 0.0f;

    float av_i = 0.0f;
    float av_q = 0.0f;

    /*
     * Find maximum absolute I/Q value for one symbol length
     */
    for (int i = 0; i < FRAME_SIZE; i += CYCLES) {
        for (int j = 0; j < CYCLES; j++) {
            av_i += fabsf(crealf(input_frame[i+j]));
            av_q += fabsf(cimagf(input_frame[i+j]));
        }
        
        av_i /= CYCLES;
        av_q /= CYCLES;

        if (av_i > max_i) {
            max_i = av_i;
        }

        if (av_q > max_q) {
            max_q = av_q;
        }

        /*
         * Create I/Q amplitude histograms
         */
        float hv_i = (max_i / 8.0f);
        float hv_q = (max_q / 8.0f);
    
        for (int k = 1; k < 8; k++) {
            if (av_i <= (hv_i * k)) {
                hist_i[k] += 1;
                break;
            }
        }

        for (int k = 1; k < 8; k++) {
            if (av_q <= (hv_q * k)) {
                hist_q[k] += 1;
                break;
            }
        }
    }

    int hmax = 0;
    int index = 0;

    /*
     * Sum the I/Q histograms
     * and index the maximum value
     */
    for (int i = 1; i < 8; i++) {            
        hist[i] = (hist_i[i] + hist_q[i]);

        if (hist[i] > hmax) {
            hmax = hist[i];
            index = i;
        }
    }

    /*
     * Decimate by CYCLES, at this point we have the 1200 Baud QPSK signal.
     *
     * Adjust for the timing error using index
     */
    for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {			// 512 / 8 = 64
        int extended = (FRAME_SIZE / CYCLES) + i;			// compute once
        
        decimated_frame[i] = decimated_frame[extended];			// use previous frame
#ifdef BUGGY
        decimated_frame[extended] = input_frame[(i * CYCLES) + index];	// current frame
#endif

/*=======================================
  =======================================

TODO - Bug, my offset index finder doesn't seem to work
As you can tell, if you put in 6 it is accurate

  ==================================================
  ================================================*/

        decimated_frame[extended] = input_frame[(i * CYCLES) + 6];
    }

    if (get_costas_enable() == true) {
        /*
         * Costas Loop over the decimated frame
         */
        for (int i = 0, j = 0; i < (FRAME_SIZE / CYCLES); i++, j += 2) {
            costas_frame[i] = decimated_frame[i] * cmplxconj(get_phase());

#ifdef TEST_SCATTER
            fprintf(stderr, "%f %f\n", crealf(costas_frame[i]), cimagf(costas_frame[i]));
#endif

            d_error = phase_detector(costas_frame[i]);

            advance_loop(d_error);
            phase_wrap();
            frequency_limit();
        
            qpsk_demod(costas_frame[i], &bits[j]);

            //printf("%d%d ", bits[j], bits[j+1]);
        }
    } else {
        for (int i = 0, j = 0; i < (FRAME_SIZE / CYCLES); i++, j += 2) {
#ifdef TEST_SCATTER
            fprintf(stderr, "%f %f\n", crealf(decimated_frame[i]), cimagf(decimated_frame[i]));
#endif
            qpsk_demod(decimated_frame[i], &bits[j]);

            //printf("%d%d ", bits[j], bits[j+1]);
        }
    }

    /*
     * Save the detected frequency error
     */
    fbb_offset_freq = (get_frequency() * RS / TAU);	// convert radians to freq at symbol rate
}

/*
 * Modulate the 1200 sym/s. First re-sample at 9600 samples/s inserting zero's,
 * then post filtered using the root raised cosine FIR, then translated to
 * 1500 Hz center frequency.
 */
static int tx_frame(int16_t samples[], complex float symbol[], int length) {
    int n = (length * CYCLES);
    complex float signal[n];	// big enough for 8x sample rate

    /*
     * Build the 1200 sym/s packet Frame by zero padding
     * for the desired (8x nyquist) 9600 sample rate.
     *
     * signal[] array length is now 8x longer.
     */
    for (int i = 0; i < length; i++) {
        int index = (i * CYCLES);	// compute once

        signal[index] = symbol[i];

        for (int j = 1; j < CYCLES; j++) {
            signal[index + j] = 0.0f;
        }
    }

    /*
     * Raised Root Cosine Filter at Baseband and 9600 samples/s.
     * This optimizes digital data signals
     */
    rrc_fir(tx_filter, signal, n);

    /*
     * Shift Baseband to 1500 Hz Center Frequency
     */
    for (int i = 0; i < n; i++) {
        fbb_tx_phase *= fbb_tx_rect;
        signal[i] *= fbb_tx_phase;
    }

    fbb_tx_phase /= cabsf(fbb_tx_phase); // normalize as magnitude can drift

    /*
     * Now return the resulting real samples
     * (imaginary part discarded for radio transmit)
     */
    for (int i = 0; i < n; i++) {
        samples[i] = (int16_t) (crealf(signal[i]) * 16384.0f); // I at @ .5
    }

    return n;
}

/*
 * Gray coded QPSK modulation function
 */
static complex float qpsk_mod(int bits[]) {
    return constellation[(bits[1] << 1) | bits[0]];
}

static int qpsk_packet_mod(int16_t samples[], int tx_bits[], int length) {
    complex float symbol[length];
    int dibit[2];

    for (int i = 0, s = 0; i < length; i++, s += 2) {
        dibit[0] = tx_bits[s + 1] & 0x1;
        dibit[1] = tx_bits[s ] & 0x1;

        symbol[i] = qpsk_mod(dibit);
    }

    return tx_frame(samples, symbol, length/2);
}

// Main Program

int main(int argc, char** argv) {
    int bits[6400];
    int16_t frame[FRAME_SIZE];
    int length;

    srand(time(0));

    /*
     * All terms are radians per sample.
     *
     * The loop bandwidth determins the lock range
     * and should be set around TAU/100 to TAU/200
     */
    create_control_loop((TAU / 200.0f), -1.6f, 1.6f);

    //set_costas_enable(false);

    /*
     * Create an RRC filter using the
     * Sample Rate, baud, and Alpha
     */
    rrc_make(FS, RS, .35f);

    /*
     * create the QPSK data waveform.
     * This simulates the transmitted packets.
     */
    fout = fopen(TX_FILENAME, "wb");

    fbb_tx_phase = cmplx(0.0f);
    //fbb_tx_rect = cmplx(TAU * CENTER / FS);
    //fbb_offset_freq = CENTER;

    fbb_tx_rect = cmplx(TAU * (CENTER + 50.0) / FS);	// 50 Hz Frequency Error
    fbb_offset_freq = (CENTER + 50.0);

    for (int k = 0; k < 100; k++) {
        // 256 QPSK
        for (int i = 0; i < FRAME_SIZE; i += 2) {
            bits[i] = rand() % 2;
            bits[i + 1] = rand() % 2;
        }

        length = qpsk_packet_mod(frame, bits, FRAME_SIZE);

        fwrite(frame, sizeof (int16_t), length, fout);
    }

    fclose(fout);

    /*
     * Now try to process what was transmitted
     */
    fin = fopen(TX_FILENAME, "rb");

    fbb_rx_phase = cmplx(0.0f);
    fbb_rx_rect = cmplxconj(TAU * CENTER / FS);

    while (1) {
        /*
         * Read in the frame samples
         */
        size_t count = fread(frame, sizeof (int16_t), FRAME_SIZE, fin);

        if (count != FRAME_SIZE)
            break;

        rx_frame(frame, bits);
    }
    
    fclose(fin);

    return (EXIT_SUCCESS);
}
