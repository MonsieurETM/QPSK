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
#include "costas_loop.h"
#include "rrc_fir.h"

// Prototypes

static float phase_detector(complex float);
static void qpsk_demod(complex float, int []);
static void rx_frame(int16_t []);
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
complex float decimated_frame[FRAME_SIZE / 2];
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

static float phase_detector(complex float sample) {
    return ((crealf(sample) > 0.0f ? 1.0f : -1.0f) * cimagf(sample) -
            (cimagf(sample) > 0.0f ? 1.0f : -1.0f) * crealf(sample));
}

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
    complex float rotate = symbol * cmplx(ROTATE45);

    bits[0] = crealf(rotate) < 0.0f; // I < 0 ?
    bits[1] = cimagf(rotate) < 0.0f; // Q < 0 ?
}

/*
 * Receive function
 * 
 * 2400 baud QPSK at 9600 samples/sec.
 *
 * Remove any frequency and timing offsets
 */
static void rx_frame(int16_t in[]) {
    float max_i = 0.0f;
    float max_q = 0.0f;

    float av_i = 0.0f;;
    float av_q = 0.0f;

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
    int bits[2];

    /*
     * Convert input PCM to complex samples
     * at 9600 Hz sample rate
     */
    for (int i = 0; i < FRAME_SIZE; i++) {
        fbb_rx_phase *= fbb_rx_rect;

        input_frame[i] = fbb_rx_phase * ((float) in[i] / 16384.0f);
    }

    fbb_rx_phase /= cabsf(fbb_rx_phase); // normalize as magnitude can drift

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

    /*
     * Costas Loop over the decimated frame
     */
    for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
       costas_frame[i] = decimated_frame[i] * cmplxconj(get_phase());

#ifdef TEST_SCATTER
        fprintf(stderr, "%f %f\n", crealf(costas_frame[i]), cimagf(costas_frame[i]));
#endif

        d_error = phase_detector(costas_frame[i]);

        advance_loop(d_error);
        phase_wrap();
        frequency_limit();
        
        qpsk_demod(costas_frame[i], bits);

        //printf("%d%d ", bits[0], bits[1]);
    }

    /*
     * Save the detected frequency error
     */
    fbb_offset_freq = (get_frequency() * RS / TAU);	// convert radians to freq at symbol rate
}

/*
 * Modulate the symbols by first upsampling to 9600 Hz sample rate,
 * and translating the spectrum to 1500 Hz, where it is filtered
 * using the root raised cosine coefficients.
 */
static int tx_frame(int16_t samples[], complex float symbol[], int length) {
    complex float signal[(length * CYCLES)];

    /*
     * Build the 2400 baud packet Frame zero padding
     * for the desired 9600 Hz sample rate.
     */
    for (int i = 0; i < length; i++) {
        signal[(i * CYCLES)] = symbol[i];

        for (int j = 1; j < CYCLES; j++) {
            signal[(i * CYCLES) + j] = 0.0f;
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
        samples[i] = (int16_t) (crealf(signal[i]) * 16384.0f); // I at @ .5
    }

    return (length * CYCLES);
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

    return tx_frame(samples, symbol, length);
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
     * and should be set around 2pi/100 to 2pi/200
     */
    create_control_loop((TAU / 200.0f), -1.6f, 1.6f);

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

    fbb_tx_rect = cmplx(TAU * (CENTER + 5.0) / FS);
    fbb_offset_freq = (CENTER + 5.0);

    for (int k = 0; k < 100; k++) {
        // 256 QPSK
        for (int i = 0; i < FRAME_SIZE; i += 2) {
            bits[i] = rand() % 2;
            bits[i + 1] = rand() % 2;
        }

        length = qpsk_packet_mod(frame, bits, (FRAME_SIZE / 2));

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

        rx_frame(frame);
    }
    
    fclose(fin);

    return (EXIT_SUCCESS);
}
