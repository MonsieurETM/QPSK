# Makefile for QPSK test modem

SRC=qpsk.c rrc_fir.c interpolatingSampleBuffer.c costas-loop.c QPSKDemodulator.c symbolEvaluator.c
HEADER=qpsk.h rrc_fir.h interp.h costas-loop.h

qpsk: ${SRC} ${HEADER}
	gcc -std=c11 ${SRC} -DTEST_SCATTER -o qpsk -Wall -lm

# generate scatter diagram PNG
test_scatter: qpsk
	./qpsk 2>scatter.txt
	DISPLAY="" octave-cli -qf --eval "load scatter.txt; plot(scatter(1000:2000,1),scatter(1000:2000,2),'.'); print('scatter.png','-dpng')"
  
