/*
 * qpsk.c
 *
 * Testing program for qpsk modem algorithms, December 2022
 * Designed for 2400 Baud, 4800 bit/s
 *
 * Portions Copyright (C) 2022 Fadi Jerji
 *
 * License: GNU Affero General Public License version 3
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
#include "fft.h"

// Externs

extern complex double *constellation;

extern int nBits;
extern int nSymbols;

// Prototypes

static void rx_frame(int16_t [], double []);
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

int time_sync_Nsymb;

void symbol_mod(complex double *in, complex double *out) {
	ifft(in, out);
}

void symbol_demod(complex double *in, complex double *out) {
	fft(in, out);
}

double frequency_sync(complex double *in, double carrier_freq_width) {
	double frequency_offset_prec = 0.;

	complex double frame[NFFT];
	complex double frame_fft1[NFFT];
	complex double frame_fft2[NFFT];

	for (int i = 0; i < NFFT/2; i++) {
		frame[i] = *(in + i);
		frame[i + NFFT/2] = *(in + i);
	}

	fft(frame, frame_fft1);

	for (int i = 0; i < NFFT/2; i++) {
		frame[i] = *(in + i + NFFT/2);
		frame[i + NFFT/2] = *(in + i + NFFT/2);
	}

	fft(frame, frame_fft2);

        complex double mul = 0.;

	for (int i = 0; i < NFFT; i++) {
	    mul += conj(frame_fft2[i]) * frame_fft1[i];
        }

	frequency_offset_prec = 2.0 * (1.0 / TAU) * carg(mul);

	return (frequency_offset_prec * carrier_freq_width);
}

complex double interpolate_linear(complex double a,double a_x,complex double b,double b_x,double x)
{
	return a + (b - a) * (x - a_x) / (b_x - a_x);
}

void rational_resampler(complex double *in, int in_size, complex double *out, int rate, int interpolation_decimation) {
	if (interpolation_decimation == DECIMATION) {
		int index = 0;
		
		for (int i = 0; i < in_size; i += rate) {
			*(out+index) = *(in+i);
			index++;
		}
	} else if (interpolation_decimation == INTERPOLATION) {
		for (int i = 0; i < in_size-1; i++) {
			for(int j = 0; j < rate; j++) {
				*(out + i * rate + j) = interpolate_linear(*(in+i),0,*(in+i+1),rate,j);
			}
		}

		for (int j = 0; j < rate; j++) {
			*(out + (in_size - 1) * rate + j) = interpolate_linear(*(in+in_size-2),0,*(in+in_size-1),rate,rate+j);
		}
	}
}

void baseband_to_passband(complex double *in, int in_size, double *out,
    double sampling_frequency, double carrier_frequency, double carrier_amplitude,
    int interpolation_rate) {
	double sampling_interval = 1.0 / sampling_frequency;
	complex double data_interpolated[in_size * interpolation_rate];

	rational_resampler(in, in_size, data_interpolated, interpolation_rate, INTERPOLATION);

	for (int i = 0; i < (in_size * interpolation_rate); i++) {
	    double temp_a = data_interpolated[i] * carrier_amplitude;
	    double temp_f = TAU * carrier_frequency * (double)i * sampling_interval;

            out[i] = (temp_a * cos(temp_f)) + (temp_a * sin(temp_f));	// this is not complex
	}
}

void passband_to_baseband(double *in, int in_size, complex double *out,
    double sampling_frequency, double carrier_frequency, double carrier_amplitude,
    int decimation_rate, double *filter_coefficients, int filter_nTaps) {
	double sampling_interval = 1.0 / sampling_frequency;
	complex double l_data[in_size];
	complex double data_filtered[in_size + filter_nTaps - 1];

	for (int i = 0; i < in_size; i++) {
	    double temp_a = in[i] * carrier_amplitude;
	    double temp_f = TAU * carrier_frequency * (double)i * sampling_interval;
            
            l_data[i] = (temp_a * cos(temp_f)) + (temp_a * sin(temp_f)) * I;
	}

	for (int i = 0; i < (in_size + filter_nTaps - 1); i++) {
		complex double acc = 0.;

		for (int j = 0; j < filter_nTaps; j++) {
			if ((i - j) >= 0 && (i - j) < in_size) {
				acc += l_data[i - j] * filter_coefficients[j];
			}
		}

		data_filtered[i] = acc;
	}

	rational_resampler(&data_filtered[(int)(filter_nTaps - 1) / 2], in_size, out, decimation_rate, DECIMATION);
}

int time_sync(complex double *in, int size, int interpolation_rate, int location_to_return) {
	int corss_corr_loc[size];
	double corss_corr_vals[size];

	for (int i = 0; i < size; i++) {
		corss_corr_loc[i] = -1;
		corss_corr_vals[i] = 0.;
	}

	for (int i = 0; i < (size - NFFT * interpolation_rate); i++) {
		complex double *a_c = in + i;
		complex double *b_c = in + i + NFFT * interpolation_rate;
		double corss_corr=0.;

		double norm_a = 0.;
		double norm_b = 0.;

		for (int j = 0; j < nSymbols; j++) {
			if (j < time_sync_Nsymb) {
				for (int m = 0; m < interpolation_rate; m++) {
					corss_corr += creal(a_c[m + j * NFFT * interpolation_rate]) * creal(b_c[m + j * NFFT * interpolation_rate]);

					norm_a += creal(a_c[m + j * NFFT * interpolation_rate]) * creal(a_c[m + j * NFFT * interpolation_rate]);
					norm_b += creal(b_c[m + j * NFFT * interpolation_rate]) * creal(b_c[m + j * NFFT * interpolation_rate]);

					corss_corr += cimag(a_c[m + j * NFFT * interpolation_rate]) * cimag(b_c[m + j * NFFT * interpolation_rate]);

					norm_a += cimag(a_c[m + j * NFFT * interpolation_rate]) * cimag(a_c[m + j * NFFT * interpolation_rate]);
					norm_b += cimag(b_c[m + j * NFFT * interpolation_rate]) * cimag(b_c[m + j * NFFT * interpolation_rate]);
				}
			}
		}

		corss_corr /= sqrt(norm_a * norm_b);
		corss_corr_vals[i] = corss_corr;
		corss_corr_loc[i] = i;
	}

	for (int i = 0; i < (size - NFFT * interpolation_rate - 1); i++) {
		for (int j = 0; j < (size - NFFT * interpolation_rate - 1);j++) {
			if (corss_corr_vals[j] < corss_corr_vals[j+1]) {
				double tmp = corss_corr_vals[j];
				corss_corr_vals[j] = corss_corr_vals[j+1];
				corss_corr_vals[j+1] = tmp;

				int tmp_int = corss_corr_loc[j];
				corss_corr_loc[j] = corss_corr_loc[j+1];
				corss_corr_loc[j+1] = tmp_int;
			}
		}
	}

	return corss_corr_loc[location_to_return];
}


/*
 * Receive function
 * 
 * 2400 baud/4800 bit/s QPSK at 9600 samples/sec.
 *
 * Remove any frequency and timing offsets
 */
static void rx_frame(int16_t in[], double output[]) {
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

        psk_demod(costas_frame, (FRAME_SIZE / CYCLES), output, 2.6);
    } else {
        for (int i = 0; i < (FRAME_SIZE / CYCLES); i++) {
#ifdef TEST_SCATTER
            fprintf(stderr, "%f %f\n", creal(decimated_frame[i]), cimag(decimated_frame[i]));
#endif
        }

        psk_demod(decimated_frame, (FRAME_SIZE / CYCLES), output, 2.6);
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

    time_sync_Nsymb = 1;

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
    set_constellation(MOD_QPSK);

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

    double output[(FRAME_SIZE / CYCLES)];

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

