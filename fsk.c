#include "rtl_rfm.h"
#include "fsk.h"

#include "mavg.h"

int samplerate = BIGSAMPLERATE/DOWNSAMPLE;/*31200;*/ /*19200;*/ //38400; // multiple of baudrate
int windowsize;
float fc; // Fc = mavg filter cutoff frequency. Aim for baud/10
float fc2;
int filtersize; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc
int filter2size;

Mavg filter, filter2, filter3;

int32_t mavg;

void fsk_init() {
	//samplerate = baudrate * windowsize;
	windowsize = samplerate / baudrate;
	fc = baudrate / 32; // Fc = mavg filter curoff frequency. Aim for baud/16?
	filtersize = (0.443 * samplerate) / fc; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc

	fc2 = baudrate * 1.0; /// 1.5;
	filter2size = ((float) (0.443 * (float) samplerate)) / fc2;
	filter2size = (filter2size < 1) ? 1 : filter2size;

	//filter2size = 10; // temp. calcuate properly. baud rate * 2?

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

// START OF HIGHPASS FILTER

int16_t offsethold = 0;
bool hold = false;

int16_t hipass(int16_t sample) {
	if (!hold) offsethold = process(&filter, sample) / filtersize;

	return sample - offsethold;
}

// END OF HIGHPASS FILTER

// START OF LOWPASS FILTER

int16_t lopass(int16_t sample) {

	return process(&filter2, sample) / filter2size;
}

// END OF LOWPASS FILTER

// START OF MOVING AVERAGE

int32_t moving_average(int16_t sample) {

	return process(&filter3, sample);

}

// END OF MOVING AVERAGE

void print_waveform(int16_t sample, int16_t magnitude) {
	int x = 100 + (100 * sample / (INT16_MAX));
	x = (x < 0) ? 0 : x;
	x = (x > 200) ? 200 : x;
	//printf("%08d", sample);
	for (int i = 0; i < x; i++) putchar(i == 100 ? '.' : ' ');
	putchar('x');
	for (int i = x+1; i < 200; i++) putchar(i == 100 ? '.' : ' ');

	printf("%08i", magnitude);
	//putchar('\t');
}

#define CLKPERIOD windowsize
int clk = 1;
int16_t thissample = 0;
int16_t prevsample = 0;

int8_t fsk_decode(int16_t sample, int16_t magnitude) {

	clk = (clk + 1) % CLKPERIOD;

	prevsample = thissample; // record previous sample for the purposes of zero-crossing detection
	thissample = lopass(hipass(sample));

	mavg = moving_average(thissample);

	if (debugplot) print_waveform(thissample, magnitude);
	
	// Zero-Crossing Detector:
	if ((thissample < 0 && prevsample >= 0) || (thissample > 0 && prevsample <= 0)) {
		if (debugplot) printf("K");

		if (clk > 0 && clk <= (CLKPERIOD/2)) {
			clk -= (CLKPERIOD+1); // delay clock	
			if (debugplot) printf ("^%d", clk); 		// clock has happened recently
		} else if (clk > (CLKPERIOD/2) && clk < (CLKPERIOD)) {
			clk = (clk + 1) % CLKPERIOD;			// advance clock 
			if (debugplot) printf("v%d", clk); 		// clock is happening soon
		} else {
			if (debugplot) putchar('<');		// clock is locked on
		}		
	}

	uint8_t thebit = (mavg > 0) ? 1 : 0; // take the sign of the moving average window. Effectively a low pass with binary threshold...

	if (debugplot) putchar('\n');

	if (clk == 0) return(thebit);	
	else return -1;
	


}