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
complex float decimated_frame[FRAME_SIZE / 2];
complex float costas_frame[FRAME_SIZE / CYCLES];

// Two phase for full duplex

complex float fbb_tx_phase;
complex float fbb_tx_rect;

complex float fbb_rx_phase;
complex float fbb_rx_rect;

float fbb_offset_freq;

float d_error;

#ifdef FUTURE
/*
 * Resampler for 8 kHz to 48 kHz
 * Generate using fir1(47,1/6) in Octave
 */
static const float filter48[] = {
    -3.55606818e-04,
    -8.98615286e-04,
    -1.40119781e-03,
    -1.71713852e-03,
    -1.56471179e-03,
    -6.28128960e-04,
    1.24522223e-03,
    3.83138676e-03,
    6.41309478e-03,
    7.85893186e-03,
    6.93514929e-03,
    2.79361991e-03,
    -4.51051400e-03,
    -1.36671853e-02,
    -2.21034939e-02,
    -2.64084653e-02,
    -2.31425052e-02,
    -9.84218694e-03,
    1.40648474e-02,
    4.67316298e-02,
    8.39615986e-02,
    1.19925275e-01,
    1.48381174e-01,
    1.64097819e-01,
    1.64097819e-01,
    1.48381174e-01,
    1.19925275e-01,
    8.39615986e-02,
    4.67316298e-02,
    1.40648474e-02,
    -9.84218694e-03,
    -2.31425052e-02,
    -2.64084653e-02,
    -2.21034939e-02,
    -1.36671853e-02,
    -4.51051400e-03,
    2.79361991e-03,
    6.93514929e-03,
    7.85893186e-03,
    6.41309478e-03,
    3.83138676e-03,
    1.24522223e-03,
    -6.28128960e-04,
    -1.56471179e-03,
    -1.71713852e-03,
    -1.40119781e-03,
    -8.98615286e-04,
    -3.55606818e-04
};
#endif

/*
 * QPSK Quadrant bit-pair values - Gray Coded
 */
const complex float constellation[] = {
    1.0f + 0.0f * I, //  I
    0.0f + 1.0f * I, //  Q
    0.0f - 1.0f * I, // -Q
    -1.0f + 0.0f * I // -I
};

#ifdef FUTURE

// Think about using 8 kHz rate versus 9600

/*
 * Changes the sample rate of a signal from 8 kHz to 48 kHz.
 *
 * n is the number of samples at the 8 kHz rate, there are OS_48 * n samples
 * at the 48 kHz rate.  A memory of OS_TAPS_48/OS_48 samples is reqd for in8k[]
 */
void resample_8_to_48(float out48k[], float in8k[], int n)
{
    for (int i = 0; i < n; i++) {
	for (int j = 0; j < OS_48; j++) {
	    out48k[i * OS_48 + j] = 0.0f;
	    
	    for (int k = 0, l = 0; k < OS_TAPS_48K; k += OS_48, l++)
		out48k[i * OS_48 + j] += filter48[k + j] * in8k[i - l];
	    
	    out48k[i * OS_48 + j] *= OS_48;
	}
    }	

    /* update filter memory */

    for (int i = -OS_TAPS_48_8K; i < 0; i++)
	in8k[i] = in8k[i + n];
}

/*
 * Changes the sample rate of a signal from 48 to 8 kHz.
 *
 * n is the number of samples at the 8 kHz rate, there are OS_48 * n
 * samples at the 48 kHz rate.  As above however a memory of
 * OS_TAPS_48 samples is reqd for in48k[]
 */
void resample_48_to_8(float out8k[], float in48k[], int n)
{
    for (int i = 0; i < n; i++) {
	out8k[i] = 0.0f;

	for (int j = 0; j < OS_TAPS_48K; j++)
	    out8k[i] += filter48[j] * in48k[i * OS_48 - j];
    }

    /* update filter memory */

    for (int i = -OS_TAPS_48K; i < 0; i++)
	in48k[i] = in48k[i + n * OS_48];
}
#endif

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
 * 2400 baud QPSK at 9600 samples/sec.
 *
 * Remove any frequency and timing offsets
 *
 * NOTE: This whole histogram algorith is highly suspect
 *       It doesn't work with different symbol rates.
 */
static void rx_frame(int16_t in[], int bits[]) {
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
 * Modulate the 2400 sym/s. First re-sample at 9600 samples/s inserting zero's,
 * then post filtered using the root raised cosine FIR, then translated to
 * 1500 Hz center frequency.
 */
static int tx_frame(int16_t samples[], complex float symbol[], int length) {
    int n = (length * CYCLES);
    complex float signal[n];	// big enough for 4x sample rate

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
            signal[index + j] = 0.0f;
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

        // Send 256 dibits
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

        rx_frame(frame, bits);
    }
    
    fclose(fin);

    return (EXIT_SUCCESS);
}

