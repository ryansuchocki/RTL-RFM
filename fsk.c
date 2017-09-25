#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "rtl_rfm.h"
#include "fsk.h"

int samplerate = BIGSAMPLERATE/DOWNSAMPLE;/*31200;*/ /*19200;*/ //38400; // multiple of baudrate
int windowsize;
float fc; // Fc = mavg filter cutoff frequency. Aim for baud/10
float fc2;
int filtersize; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc
int filter2size;

int16_t *filter;
int16_t *filter2;
int16_t *mavgbuffer;

int32_t mavg;

void fsk_init() {
	//samplerate = baudrate * windowsize;
	windowsize = samplerate / baudrate;
	fc = baudrate / 32; // Fc = mavg filter curoff frequency. Aim for baud/16?
	filtersize = (0.443 * samplerate) / fc; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc

	fc2 = baudrate * 1.5; /// 1.5;
	filter2size = ((float) (0.443 * (float) samplerate)) / fc2;
	filter2size = (filter2size < 1) ? 1 : filter2size;

	//filter2size = 10; // temp. calcuate properly. baud rate * 2?

	if (!quiet) printf(">> Setting filters at '%.2fHz < signal < %.2fHz' (hipass size: %i, lopass size: %i, window size: %i)\n", fc, fc2, filtersize, filter2size, windowsize);

	if (!quiet) printf(">> RXBw is %.1fkHz around %.4fMHz.\n\n", (float)samplerate/1000.0, (float)freq/1000000.0);

	filter = malloc(sizeof(int16_t) * filtersize);
	filter2 = malloc(sizeof(int16_t) * filter2size);
	mavgbuffer = malloc(sizeof(int16_t) * windowsize);

	for (int i = 0; i < filtersize; i++) filter[i] = 0;
	for (int i = 0; i < filter2size; i++) filter2[i] = 0;
	for (int i = 0; i < windowsize; i++) mavgbuffer[i] = 0;
}

void fsk_cleanup() {
	free(filter);
	free(filter2);
	free(mavgbuffer);
}

// START OF HIGHPASS FILTER

int fi = 0;
int32_t count = 0;

int16_t latestoffset = 0;
int16_t offsethold = 0;
bool hold = false;

int16_t hipass(int16_t sample) {
	if (hold) return sample - offsethold;

	fi = (fi + 1) % filtersize;

	count -= filter[fi];
	filter[fi] = sample;
	count += filter[fi];

	latestoffset = count / filtersize;

	return sample - latestoffset;
}

// END OF HIGHPASS FILTER

// START OF LOWPASS FILTER

int f2i = 0;
int32_t count2 = 0;

int16_t lopass(int16_t sample) {

	if (filter2size < 2) return sample;

	f2i = (f2i + 1) % filter2size;

	count2 -= filter2[f2i];
	filter2[f2i] = sample;
	count2 += filter2[f2i];

	return (count2 / filter2size);
}

// END OF LOWPASS FILTER

// START OF MOVING AVERAGE

short bi = 0;
int32_t mavgcount = 0;

int32_t moving_average(int16_t sample) {

	bi = (bi + 1) % windowsize;

	mavgcount -= mavgbuffer[bi];
	mavgbuffer[bi] = sample;
	mavgcount += mavgbuffer[bi];

	return mavgcount /*/ windowsize*/;
}

// END OF MOVING AVERAGE

void print_waveform(int16_t sample, int16_t magnitude) {
	/*if (magnitude < 10) {		
		return;
	}*/

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

	//uint8_t b1 = fgetc(rtlstream);
	//uint8_t b2 = fgetc(rtlstream);

	prevsample = thissample; // record previous sample for the purposes of zero-crossing detection
	//thissample = lopass(hipass((int16_t) (b1 | (b2<<8)) /*/ 2*/)); // make headroom for filter offset. Unnecessary! filter always moves it towards zero!
	thissample = lopass(hipass(sample));

	mavg = moving_average(thissample);

	//if (debugplot) print_waveform(thissample, magnitude);
	
	// Zero-Crossing Detector:
	if ((thissample < 0 && prevsample >= 0) || (thissample > 0 && prevsample <= 0)) {
		//if (debugplot) printf("K");

		if (clk > 0 && clk <= (CLKPERIOD/2)) {
			clk -= (CLKPERIOD+1); // delay clock
			//clk = clk - 1;		
			//if (clk == 0) clk = -1*CLKPERIOD;						// delay clock		
			//if (debugplot) printf ("^%d", clk); 		// clock has happened recently
		} else if (clk > (CLKPERIOD/2) && clk < (CLKPERIOD)) {
			clk = (clk + 1) % CLKPERIOD;			// advance clock 
			//if (debugplot) printf("v%d", clk); 		// clock is happening soon
		} else {
			//if (debugplot) putchar('<');		// clock is locked on
		}		
	}

	uint8_t thebit = (mavg > 0) ? 1 : 0; // take the sign of the moving average window. Effectively a low pass with binary threshold...

	//if (debugplot) putchar('\n');


	if (clk == 0) return(thebit);	
	else return -1;
	


}