#### QPSK - An Experimental QPSK Packet Data Modem
This project is to design a modem which does not require a preamble, or a Unique Word (UW) in order to synchronize the timing. There may be a sync word as part of a protocol, but not for timing. I'm interested in making it work for QPSK 2400 Baud VHF/UHF band, and 1200 Baud for 10 Meters. This is an alternative to FM modems.

At this stage my timing estimation code is unreliable.

There also needs to be a frequency error adjustment. Not so much for fixed radio sites, as they don't move, but mobiles will have a varying Doppler shift to be corrected. So I ported the GNU Radio Costas loop C++ code to C and merged that with the receiver code.

To compile and make the ```qpsk``` binary, just type ```make``` or if you want to see the scatter diagram graphic ```make test_scatter```

There's a ```scatter.png``` to show the best decode, but it is hit and miss.

The costas does detect the correct frequency error and the scatter plot does seem to plot correctly, but you have to play around with the loop bandwidth values from TAU/100 to TAU/200.

