/*
 * qpsk.c
 *
 * Testing program for qpsk modem algorithms, January 2023
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
#include "costas-loop.h"
#include "rrc_fir.h"
#include "interp.h"

// Prototypes

static void rx_frame(int16_t *, int[]);
static complex double qpsk_mod(int[]);
static int tx_frame(int16_t[], complex double[], int);
static complex double qpsk_mod(int[]);
static int qpsk_packet_mod(int16_t[], int[], int);

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
 *
 * 00 is east (Diamond)
 */
static const complex double constellation[] = {
    1.0 + 0.0 * I, //  East
    0.0 + 1.0 * I, //  North
    0.0 - 1.0 * I, //  South
    -1.0 + 0.0 * I //  West
};

/*
 * Experimental option
 *
 * 00 is north-west (Rectangular)
 *
static const complex double constellation[] = {
   -1.0 + 1.0 * I, //  North-West
   -1.0 - 1.0 * I, //  South-West
    1.0 + 1.0 * I, //  North-East
    1.0 - 1.0 * I  //  South-East
};
*/

/*
 * Receive function
 *
 * 2400 baud QPSK at 9600 samples/sec.
 *
 * Remove any frequency and timing offsets
 */
static void rx_frame(int16_t *in, int bits[])
{
    /*
     * Convert input PCM to complex samples
     * to baseband
     */
    fbb_rx_phase *= fbb_rx_rect;

    complex double sample = fbb_rx_phase * ((double)*in / 16384.0f);

    /*
     * Raised Root Cosine Filter
     */
    rrc_fir(rx_filter, &sample);

    Dibit dbit = demod_receive(sample);

    if (dbit != D99)
    {
        complex double val = getReceivedSample(); // Costas compensated
#ifdef TEST_SCATTER
        fprintf(stderr, "%f %f\n", creal(val), cimag(val));
#endif
    }

    /*
     * Save the detected frequency error
     */
    fbb_offset_freq = (get_frequency() * RS / TAU); // convert radians to freq at symbol rate
    // printf("%.2f ", fbb_offset_freq);
}

/*
 * Modulate the symbols by first upsampling, then RRC filtering.
 */
static int tx_frame(int16_t samples[], complex double symbol[], int length)
{
    complex double signal[(length * CYCLES)];

    /*
     * Build the baseband packet Frame by zero padding
     * to the desired sample rate.
     */
    for (int i = 0; i < length; i++)
    {
        signal[(i * CYCLES)] = symbol[i];

        for (int j = 1; j < CYCLES; j++)
        {
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
    for (int i = 0; i < (length * CYCLES); i++)
    {
        fbb_tx_phase *= fbb_tx_rect;
        signal[i] *= fbb_tx_phase;
    }

    fbb_tx_phase /= cabsf(fbb_tx_phase); // normalize as magnitude can drift

    /*
     * Now return the resulting real samples (imaginary part discarded)
     */
    for (int i = 0; i < (length * CYCLES); i++)
    {
        complex double val = signal[i] * 16384.0;

        // Summing has the effect of rotating the constellation 45 deg
        // (rectangular rather than diamond)

        samples[i] = (int16_t)(creal(val) + cimag(val)); // @ .5
    }

    return (length * CYCLES);
}

/*
 * Gray coded QPSK modulation function
 */
static complex double qpsk_mod(int bits[])
{
    return constellation[(bits[1] << 1) | bits[0]];
}

static int qpsk_packet_mod(int16_t samples[], int tx_bits[], int length)
{
    complex double symbol[length];
    int dibit[2];

    for (int i = 0, s = 0; i < length; i++, s += 2)
    {
        dibit[0] = tx_bits[s + 1] & 0x1;
        dibit[1] = tx_bits[s] & 0x1;

        symbol[i] = qpsk_mod(dibit);
    }

    return tx_frame(samples, symbol, length);
}

// Main Program

int main(int argc, char **argv)
{
    int bits[6400];
    int16_t frame[1];
    int length;

    srand(time(0));

    /*
     * All terms are radians per sample.
     *
     * The loop bandwidth determins the lock range
     * and should be set around TAU/100 to TAU/200
     */
    create_control_loop((TAU / 200.), -1., 1.);
    create_QPSKDemodulator((FS / RS), 0.1);

    /*
     * Create an RRC filter using the
     * Sample Rate, Baud, and Alpha
     */
    rrc_make(FS, RS, .35);

    /*
     * create the QPSK data waveform.
     * This simulates the transmitted packets.
     */
    fout = fopen(TX_FILENAME, "wb");

    fbb_tx_phase = cmplx(0.0);
    // fbb_tx_rect = cmplx(TAU * CENTER / FS);
    // fbb_offset_freq = CENTER;

    fbb_tx_rect = cmplx(TAU * (CENTER + 50.0) / FS); // 50 Hz error
    fbb_offset_freq = (CENTER + 50.0);

    for (int k = 0; k < 2000; k++)
    {
        // 256 QPSK
        for (int i = 0; i < FRAME_SIZE; i++)
        {
            bits[i] = rand() % 2;
        }

        length = qpsk_packet_mod(frame, bits, (FRAME_SIZE / 2));

        fwrite(frame, sizeof(int16_t), length, fout);
    }

    fclose(fout);

    /*
     * Now try to process what was transmitted
     */
    fin = fopen(TX_FILENAME, "rb");

    fbb_rx_phase = cmplx(0.0);
    fbb_rx_rect = cmplxconj(TAU * CENTER / FS);

    while (1)
    {
        /*
         * Read in the frame samples
         */
        size_t count = fread(frame, sizeof(int16_t), 1, fin);

        if (count != 1)
            break;

        rx_frame(frame, bits);
    }

    fclose(fin);

    return (EXIT_SUCCESS);
}
