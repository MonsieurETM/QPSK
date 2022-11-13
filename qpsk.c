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

static float cnormf(complex float);
static int find_quadrant(complex float);

// Globals

static FILE *fin;
static FILE *fout;

static complex float tx_filter[NTAPS];
static complex float rx_filter[NTAPS];

static complex float input_frame[FRAME_SIZE];
static complex float costas_frame[522];		// allow for index overflow
static complex float decimated_frame[256];

// Two phase for full duplex

static complex float fbb_tx_phase;
static complex float fbb_tx_rect;

static complex float fbb_rx_phase;
static complex float fbb_rx_rect;

static float fbb_offset_freq;

static float d_error;

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

static float phase_detector(complex float sample) {
    return ((crealf(sample) > 0.0f ? 1.0f : -1.0f) * cimagf(sample) -
            (cimagf(sample) > 0.0f ? 1.0f : -1.0f) * crealf(sample));
}

static float convolve(const float *data, const float *filter, int filter_size) {
	float sum = 0.0f;

	for (int i = 0; i < filter_size; i++) {
	    sum += filter[i] * data[i];
	}

	return sum;
}

/*
 * This algorithm would be used with a
 * Viterbi decoder.
 */
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
     * Costas Loop over the whole filtered input frame
     */
    for (int i = 0; i < FRAME_SIZE; i++) {
        costas_frame[i] = input_frame[i] * cmplxconj(get_phase());

        d_error = phase_detector(costas_frame[i]);

        advance_loop(d_error);
        phase_wrap();
        frequency_limit();
    }

    /*
     * Save the detected frequency error
     */
    fbb_offset_freq = (get_frequency() * FS / TAU);
    
    //printf("Frequency Error = %.1f\n", fbb_offset_freq);

    float max_i = 0.0f;
    float max_q = 0.0f;

    float av_i = 0.0f;;
    float av_q = 0.0f;
    
    /*
     * You need as many histograms as you think
     * you'll need for timing offset. Using 8 for now.
     */
    int hist_i[8] = { 0 };
    int hist_q[8] = { 0 };
    
    /*
     * Find maximum absolute I/Q value for one symbol length
     * after passing through the filter
     */
    for (int i = 0; i < FRAME_SIZE; i += CYCLES) {
        for (int j = 0; j < CYCLES; j++) {
            av_i += fabsf(crealf(costas_frame[i+j]));
            av_q += fabsf(cimagf(costas_frame[i+j]));
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
     * Sum the I/Q histograms
     * and ind the maximum value
     */
    int hmax = 0;
    int index = 0;

    int hist[8] = { 0 };
    
    for (int i = 0; i < 8; i++) {            
        hist[i] = (hist_i[i] + hist_q[i]);

        if (hist[i] > hmax) {
            hmax = hist[i];
            index = i;
        }
    }

    //printf("Index = %d\n", index);

    /*
     * Decimate by 4 to the 2400 symbol rate
     * adjust for the timing error using index
     */
    for (int i = 0; i < FRAME_SIZE; i += CYCLES) {
        complex float symbol = costas_frame[i + index]; // costas frame should be greater than FRAME_SIZE + max index

#ifdef TEST_SCATTER
        fprintf(stderr, "%f %f\n", crealf(symbol), cimagf(symbol));
#endif

        //int quad = find_quadrant(symbol);

        //printf("%d ", quad);
    }
}

/*
 * Gray coded QPSK modulation function
 */
complex float qpsk_mod(int bits[]) {
    return constellation[(bits[1] << 1) | bits[0]];
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

    create_control_loop((M_PI / 100.0f), 1.0f, -1.0f);

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
    fbb_tx_rect = cmplx(TAU * CENTER / FS);    // add TX Freq Error to center
    fbb_offset_freq = CENTER;

    //fbb_tx_rect = cmplx(TAU * (CENTER + 5.0) / FS);
    //fbb_offset_freq = (CENTER + 5.0);

    for (int k = 0; k < 100; k++) {
        /*
         * NS data frames
         */
        for (int j = 0; j < NS; j++) {
            // 32 QPSK
            for (int i = 0; i < (DATA_SYMBOLS * 2); i += 2) {
                bits[i] = rand() % 2;
                bits[i + 1] = rand() % 2;
                
                //printf("%d%d ", bits[i], bits[i + 1]);
            }

            length = qpsk_data_modulate(frame, bits, DATA_SYMBOLS);

            fwrite(frame, sizeof (int16_t), length, fout);
        }
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
