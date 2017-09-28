#include "rtl_rfm.h"
#include "fsk.h"

#include "mavg.h"

int windowsize;

void fsk_init(int freq, int samplerate, int baudrate) {
	windowsize = samplerate / baudrate;
	float fc = baudrate / 32; // Fc = mavg filter curoff frequency. Aim for baud/16?
	int filtersize = (0.443 * samplerate) / fc; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc

	float fc2 = baudrate * 1.0; /// 1.5;
	int filter2size = ((float) (0.443 * (float) samplerate)) / fc2;
	filter2size = (filter2size < 1) ? 1 : filter2size;

	if (!quiet) printf(">> Setting filters at '%.2fHz < signal < %.2fHz' (hipass size: %i, lopass size: %i, window size: %i)\n", fc, fc2, filtersize, filter2size, windowsize);

	if (!quiet) printf(">> RXBw is %.1fkHz around %.4fMHz.\n", (float)samplerate/1000.0, (float)freq/1000000.0);

	mavg_init(&filter, filtersize);
	mavg_init(&filter2, filter2size);
	mavg_init(&filter3, windowsize);
}

void fsk_cleanup() {
	mavg_cleanup(&filter);
	mavg_cleanup(&filter2);
	mavg_cleanup(&filter3);
}


void print_waveform(int16_t sample, int16_t magnitude) {
	#define SCOPEWIDTH 128
	int x = (SCOPEWIDTH/2) + ((SCOPEWIDTH/2) * sample / (INT16_MAX));
	x = (x < 0) ? 0 : x;
	x = (x > SCOPEWIDTH) ? SCOPEWIDTH : x;
	//printf("%08d", sample);
	for (int i = 0; i < x; i++) putchar(i == (SCOPEWIDTH/2) ? '.' : ' ');
	putchar('x');
	for (int i = x+1; i < SCOPEWIDTH; i++) putchar(i == (SCOPEWIDTH/2) ? '.' : ' ');

	printf("%04i", magnitude);
	//putchar('\t');
}

int clk = 1;
int16_t prevsample = 0;

int8_t fsk_decode(int16_t sample, int16_t magnitude) {

	clk = (clk + 1) % windowsize;

	int16_t thissample = mavg_lopass(&filter2, mavg_hipass(&filter, sample));

	if (debugplot) print_waveform(thissample, magnitude);
	
	// Zero-Crossing Detector for phase correction:
	if ((thissample < 0 && prevsample >= 0) || (thissample > 0 && prevsample <= 0)) {
		if (debugplot) printf("K");

		if (clk > 0 && clk <= (windowsize/2)) {
			clk -= (windowsize+1); // delay clock	
			if (debugplot) printf ("^%d", clk); 		// clock has happened recently
		} else if (clk > (windowsize/2) && clk < (windowsize)) {
			clk = (clk + 1) % windowsize;			// advance clock 
			if (debugplot) printf("v%d", clk); 		// clock is happening soon
		} else {
			if (debugplot) putchar('<');		// clock is locked on
		}		
	}

	if (debugplot) if (clk == 0) printf("\t%d", ((mavg_count(&filter3, thissample) > 0) ? 1 : 0));	

	if (debugplot) putchar('\n');

	prevsample = thissample; // record previous sample for the purposes of zero-crossing detection

	if (clk == 0) return((mavg_count(&filter3, thissample) > 0) ? 1 : 0);	
	else return -1;
	
}