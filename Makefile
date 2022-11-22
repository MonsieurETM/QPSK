# Makefile for QPSK modem

SRC=qpsk.c costas_loop.c rrc_fir.c
HEADER=qpsk.h costas_loop.h rrc_fir.h

qpsk: ${SRC} ${HEADER}
	gcc -std=c11 ${SRC} -DTEST_SCATTER -o qpsk -Wall -lm -lresample

# generate scatter diagram PNG
test_scatter: qpsk
	./qpsk 2>scatter.txt
	DISPLAY="" octave-cli -qf --eval "load scatter.txt; plot(scatter(1000:2000,1),scatter(1000:2000,2),'.'); print('scatter.png','-dpng')"
  
