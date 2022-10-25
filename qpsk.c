/*
 * qpsk.c
 *
 * Testing program for qpsk modem algorithms, October 2022
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

static float cnormf(complex float);
static int find_quadrant(complex float);

// Globals

static FILE *fin;
static FILE *fout;

static complex float tx_filter[NTAPS];
static complex float rx_filter[NTAPS];
static complex float input_frame[(FRAME_SIZE * 2)];
static complex float decimated_frame[256];

// Two phase for full duplex

static complex float fbb_tx_phase;
static complex float fbb_tx_rect;

static complex float fbb_rx_phase;
static complex float fbb_rx_rect;

/*
 * QPSK Quadrant bit-pair values - Gray Coded
 * Non-static so they can be used by other modules
 */
const complex float constellation[] = {
    1.0f + 0.0f * I, //  I
    0.0f + 1.0f * I, //  Q
    0.0f - 1.0f * I, // -Q
    -1.0f + 0.0f * I // -I
};

static float cnormf(complex float val) {
    float realf = crealf(val);
    float imagf = cimagf(val);

    return realf * realf + imagf * imagf;
}

void set_qpsk_rx_offset(float fshift) {
    float val = TAU * (fshift / FS);
    
    if (fshift < 0.0f) {
        fbb_rx_rect *= cmplxconj(val);
    } else {
        fbb_rx_rect *= cmplx(val);
    }
}

static int find_quadrant(complex float symbol) {
    float quadrant;

    /*
     * The smallest distance between constellation
     * and the symbol, is our gray coded quadrant.
     * 
     *      1
     *      |
     *  3---+---0
     *      |
     *      2
     */

    float min_value = 200.0f; // some large value

    for (int i = 0; i < 4; i++) {
        float dist = cnormf(symbol - constellation[i]);

        if (dist < min_value) {
            min_value = dist;
            quadrant = i;
        }
    }

    return quadrant;
}

/*
 * Receive function
 * 
 * 2400 baud QPSK at 9600 samples/sec.
 * Remove any frequency and timing offsets
 */

void rx_frame(int16_t in[], int bits[]) {
    float max_i = 0.0f;
    float max_q = 0.0f;

    float av_i = 0.0f;;
    float av_q = 0.0f;
    
    float phi_error_hat;

    int rxbits[2];
    int hist_i[8] = { 0 };
    int hist_q[8] = { 0 };

    /*
     * Convert input PCM to complex samples
     * at 9600 Hz sample rate
     */
    for (int i = 0; i < FRAME_SIZE; i++) {
        fbb_rx_phase *= fbb_rx_rect;
        
        complex float val = fbb_rx_phase * ((float) in[i] / 16384.0f);

        input_frame[i] = input_frame[FRAME_SIZE + i];	// Current frame
        input_frame[FRAME_SIZE + i] = val;		// Future frame
    }

    fbb_rx_phase /= cabsf(fbb_rx_phase); // normalize as magnitude can drift

    /*
     * Raised Root Cosine Filter
     */
     
    rrc_fir(rx_filter, input_frame, FRAME_SIZE);

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
         * Create 8 I/Q amplitude histograms
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
     * Sum the two histograms
     */
    int hist[8] = { 0 };
    
    for (int j = 0; j < 8; j++) {            
        hist[j] = (hist_i[j] + hist_q[j]);
    }

    /*
     * Find the maximum value in the combined histograms
     */
    int index = 0;
    int hmax = 0;

    // Find max hist value
    for (int i = 1; i < 8; i++) {
        if (hist[i] > hmax) {
            hmax = hist[i];
            index = i;
        }
    }

    /*
     * Decimate by 4 to the 2400 symbol rate
     */
    for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
        int extended = (FRAME_SIZE / CYCLES) + i; // compute once
        
        decimated_frame[i] = decimated_frame[extended];
        decimated_frame[extended] = input_frame[(i * CYCLES) + index];

        /*
         * Compute 4th-Order phase error (remove modulation)
         */

        //phi_error_hat = cargf(cpowf(decimated_frame[extended], 4.0f));

#ifdef TEST_SCATTER
        //fprintf(stderr, "%f %f\n", crealf(decimated_frame[i]), cimagf(decimated_frame[i]));
#endif
    }
    
    for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
        //printf("%d ", find_quadrant(decimated_frame[i]));

        qpsk_demod(decimated_frame[i], rxbits);
        printf("%d%d ", rxbits[0], rxbits[1]);
    }
}

/*
 * Gray coded QPSK modulation function
 */
complex float qpsk_mod(int bits[]) {
    return constellation[(bits[1] << 1) | bits[0]];
}

/*
 * Gray coded QPSK demodulation function
 *
 * By rotating received symbol 45 degrees left the
 * bits are easier to decode as they are in a specific
 * rectangular quadrant.
 * 
 * Each bit pair differs from the next by only one bit.
 */
void qpsk_demod(complex float symbol, int bits[]) {
    complex float rotate = symbol * cmplx(ROTATE45);

    bits[0] = crealf(rotate) < 0.0f; // I < 0 ?
    bits[1] = cimagf(rotate) < 0.0f; // Q < 0 ?
}

/*
 * Modulate the symbols by first upsampling to 9600 Hz sample rate,
 * and translating the spectrum to 1500 Hz, where it is filtered
 * using the root raised cosine coefficients.
 */
int tx_frame(int16_t samples[], complex float symbol[], int length) {
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

int qpsk_data_modulate(int16_t samples[], int tx_bits[], int length) {
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
    fbb_tx_rect = cmplx(TAU * CENTER / FS);

    for (int k = 0; k < 4; k++) {
        /*
         * NS data frames
         */
        for (int j = 0; j < NS; j++) {
            // 32 QPSK
            for (int i = 0; i < (DATA_SYMBOLS * 2); i += 2) {
                bits[i] = rand() % 2;
                bits[i + 1] = rand() % 2;
                
                printf("%d%d ", bits[i], bits[i + 1]);
            }

            length = qpsk_data_modulate(frame, bits, DATA_SYMBOLS);

            fwrite(frame, sizeof (int16_t), length, fout);
        }
    }

    fclose(fout);

    printf("\n\n\n");

    /*
     * Now try to process what was transmitted
     */
    fin = fopen(TX_FILENAME, "rb");

    fbb_rx_phase = cmplx(0.0f);
    fbb_rx_rect = cmplx(TAU * -CENTER / FS);

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
