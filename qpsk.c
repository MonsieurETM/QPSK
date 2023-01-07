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

// Prototypes

static void qpsk_demod(complex double, int []);
static void rx_frame(int16_t [], int []);
static complex double qpsk_mod(int []);
static int tx_frame(int16_t [], complex double [], int);
static complex double qpsk_mod(int []);
static int qpsk_packet_mod(int16_t [], int [], int);

// Globals

FILE *fin;
FILE *fout;

complex double tx_filter[NTAPS];
complex double rx_filter[NTAPS];

complex double input_frame[FRAME_SIZE];
complex double decimated_frame[FRAME_SIZE / 2];

// Two phase for full duplex

complex double fbb_tx_phase;
complex double fbb_tx_rect;

complex double fbb_rx_phase;
complex double fbb_rx_rect;

double fbb_offset_freq;

double d_error;

/*
 * QPSK Quadrant bit-pair values - Gray Coded
 */
const complex double constellation[] = {
    1.0 + 0.0 * I, //  I
    0.0 + 1.0 * I, //  Q
    0.0 - 1.0 * I, // -Q
    -1.0 + 0.0 * I // -I
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
static void qpsk_demod(complex double symbol, int bits[]) {
    symbol *= cmplx(ROTATE45);

    bits[0] = creal(symbol) < 0.0; // I < 0 ?
    bits[1] = cimag(symbol) < 0.0; // Q < 0 ?
}

/*
 * Receive function
 * 
 * 2400 baud QPSK at 9600 samples/sec.
 *
 * Remove any frequency and timing offsets
 */
static void rx_frame(int16_t in[], int bits[]) {
    double max_i = 0.0;
    double max_q = 0.0;

    double av_i = 0.0;;
    double av_q = 0.0;

    /*
     * You need as many histograms as you think
     * you'll need for timing offset. Using 8 for now.
     * First [0] index not used.
     */
    int hist_i[8] = { 0 };
    int hist_q[8] = { 0 };
    
    int hmax = 0;
    int index = 0;

    int hist[8] = { 0 };

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
     * Raised Root Cosine Filter
     */
    rrc_fir(rx_filter, input_frame, FRAME_SIZE);

    /*
     * Find maximum absolute I/Q value for one symbol length
     * after passing through the filter
     */
    for (int i = 0; i < FRAME_SIZE; i += CYCLES) {
        for (int j = 0; j < CYCLES; j++) {
            av_i += fabs(creal(input_frame[i+j]));
            av_q += fabs(cimag(input_frame[i+j]));
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
        float hv_i = (max_i / 8.0);
        float hv_q = (max_q / 8.0);
    
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

    /*
     * Sum the I/Q histograms
     * and ind the maximum value
     */
    for (int i = 0; i < 8; i++) {            
        hist[i] = (hist_i[i] + hist_q[i]);

        if (hist[i] > hmax) {
            hmax = hist[i];
            index = i;
        }
    }

    /*
     * Decimate by 4 to the 2400 symbol rate
     * adjust for the timing error using index
     */
    for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
        int extended = (FRAME_SIZE / CYCLES) + i; // compute once
        
        decimated_frame[i] = decimated_frame[extended];			// use previous frame
        decimated_frame[extended] = input_frame[(i * CYCLES) + index];	// current frame
    }

    for (int i = 0, j = 0; i < (FRAME_SIZE / CYCLES); i++, j += 2) {
#ifdef TEST_SCATTER
        fprintf(stderr, "%f %f\n", creal(decimated_frame[i]), cimag(decimated_frame[i]));
#endif
        qpsk_demod(decimated_frame[i], &bits[j]);
    }

    /*
     * Save the detected frequency error
     */
    //fbb_offset_freq = (get_frequency() * RS / TAU);	// convert radians to freq at symbol rate
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
    rrc_fir(tx_filter, signal, (length * CYCLES));

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
    int16_t frame[FRAME_SIZE];
    int length;

    srand(time(0));

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
    fbb_tx_rect = cmplx(TAU * CENTER / FS);
    fbb_offset_freq = CENTER;

    //fbb_tx_rect = cmplx(TAU * (CENTER + 5.0) / FS);
    //fbb_offset_freq = (CENTER + 5.0);

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
        size_t count = fread(frame, sizeof (int16_t), FRAME_SIZE, fin);

        if (count != FRAME_SIZE)
            break;

        rx_frame(frame, bits);
    }
    
    fclose(fin);

    return (EXIT_SUCCESS);
}
