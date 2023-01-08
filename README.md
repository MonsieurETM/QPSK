#### QPSK - An Experimental QPSK Packet Data Modem
This project is to design a modem which does not require a preamble, or a Unique Word (UW) in order to synchronize the timing. There may be a sync word as part of a protocol, but not for timing. I'm interested in making it work for QPSK 2400 Baud VHF/UHF band. This is an alternative to FM modems.

At this stage my timing estimation code is still failing.  I'm experimenting with code taken from ```sdrtrunk``` and still have a few bugs to work out.

There also needs to be a frequency error adjustment. Not so much for fixed radio sites, as they don't move, but when mobile, of course, will have a varying Doppler shift to be corrected.

I ported the GNU Radio Costas loop C++ code to C and merged that with the receiver code.

To compile and make the ```qpsk``` binary, just type ```make``` or if you want to see the scatter diagram graphic ```make test_scatter```

