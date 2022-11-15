#### QPSK - An Experimental QPSK Modem
This project is to design a 9600 bit/s QPSK 2400 Baud modem that does not require a preamble, or a Unique Word (UW) in order to synchronize the timing. There may be a sync word as part of a protocol, but not for timing.

In theory, we don't know where the QPSK time-domain symbols begin. So we average the sample rate amplitudes over a symbol cycle. Then we generate seven (7) histograms based on these sample points. The histogram with the highest count is the winner. At this point we declare the proper index to add during demodulation (after decimation) at the symbol rate (2400 Baud).

There also needs to be a frequency error adjustment. Not so much for fixed radio sites (VHF/UHF), as they don't move, but when mobile, of course, will have a Doppler shift to be corrected. I ported the GNU Radio Costas loop C++ code to C and merged that with the receiver code.

The testing program uses Octave to generate the scatter plot. This is not really needed, but used to check progress.

Here's the transmit spectrum as viewed in audacity:

<img src="spectrum.png" width="400">  

The receive costas loop is not fully debugged, but looks reasonable so far. This display is with a 5 Hz frequency error:

<img src="scatter.png" width="400">  

#### Second Pass
The development code seems to be working alright, so now the task is to create a functional modem out of it, where various length packets can be sent and received via an Application Program Interface (API).
