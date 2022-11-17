#### QPSK - An Experimental QPSK Packet Data Modem
This project is to design a modem which does not require a preamble, or a Unique Word (UW) in order to synchronize the timing. There may be a sync word as part of a protocol, but not for timing. I'm interested in making it work for 9600 bit/s QPSK 2400 Baud VHF/UHF and also 2400 bit/s QPSK 1200 Baud, for use on 10 metres HF. 

In theory, we don't know where the QPSK time-domain symbols begin. So we average the sample rate amplitudes over a symbol cycle. Then we generate seven (7) histograms based on these sample points. The histogram with the highest count is the winner. At this point we declare the proper index to add during demodulation (after decimation) at the symbol rate (2400 Baud).

There also needs to be a frequency error adjustment. Not so much for fixed radio sites (VHF/UHF), as they don't move, but when mobile, of course, will have a Doppler shift to be corrected. I ported the GNU Radio Costas loop C++ code to C and merged that with the receiver code. I also made the costas loop optional, so you can turn it off. This might be acceptable for fixed radio sites.

Here's the 9600 bit/s transmit spectrum as viewed in audacity:

<img src="spectrum.png" width="400">  

This display is with a 5 Hz frequency error and costas loop enabled:

<img src="scatter.png" width="400">  

#### Second Pass
The development code seems to be working alright, so now the task is to create a functional modem out of it, where various length packets can be sent and received via an Application Programming Interface (API).

I've lined-up a few functions needed: crc16, bit-scramble, and interleave.
