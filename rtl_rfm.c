// rtl_rfm: FSK1200 Decoder
// R. Suchocki

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
extern int errno;

FILE *rtlfm = NULL;
bool quiet = false;
bool debugplot = false;
int freq = 869412500;
int gain = 50;
int ppm = 43;

int baudrate = 4800;
int samplerate = /*31200;*/ /*19200;*/ 38400; // multiple of baudrate
int windowsize;
int fc; // Fc = mavg filter curoff frequency. Aim for baud/10
int fc2;
int filtersize; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc
int filter2size;

int16_t *filter;
int16_t *filter2;
int16_t *mavgbuffer;

int32_t mavg;

void init() {
	//samplerate = baudrate * windowsize;
	windowsize = samplerate / baudrate;
	fc = baudrate / 32; // Fc = mavg filter curoff frequency. Aim for baud/16?
	filtersize = (0.443 * samplerate) / fc; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc

	fc2 = baudrate * 2;
	filter2size = (0.443 * samplerate) / fc2;
	filter2size = (filter2size < 1) ? 1 : filter2size;

	//filter2size = 10; // temp. calcuate properly. baud rate * 2?

	if (!quiet) printf(">> Setting filters at '%iHz < signal < %iHz' (hipass size: %i, lopass size: %i, window size: %i)\n", fc, fc2, filtersize, filter2size, windowsize);

	if (!quiet) printf(">> RXBw is %.1fkHz around %.4fMHz.\n\n", (float)samplerate/1000.0, (float)freq/1000000.0);

	filter = malloc(sizeof(int16_t) * filtersize);
	filter2 = malloc(sizeof(int16_t) * filter2size);
	mavgbuffer = malloc(sizeof(int16_t) * windowsize);
}

void cleanup() {
	free(filter);
	free(filter2);
	free(mavgbuffer);
}

// START OF HIGHPASS FILTER

int fi = 0;
int32_t count = 0;

void hipass_init() {
	for (int i = 0; i < filtersize; i++) filter[i] = 0;
}

int16_t latestoffset = 0;
int16_t offsethold = 0;
bool hold = false;

int16_t hipass(int16_t sample) {

	fi = (fi + 1) % filtersize;

	count -= filter[fi];
	filter[fi] = sample;
	count += filter[fi];

	latestoffset = count / filtersize;

	if (hold) return sample - offsethold;
	return sample - latestoffset;
}

// END OF HIGHPASS FILTER

// START OF LOWPASS FILTER

int f2i = 0;
int32_t count2 = 0;

void lopass_init() {
	for (int i = 0; i < filter2size; i++) filter2[i] = 0;
}

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

void moving_average_init() {
	for (int i = 0; i < windowsize; i++) mavgbuffer[i] = 0;
}

int32_t moving_average(int16_t sample) {

	bi = (bi + 1) % windowsize;

	mavgcount -= mavgbuffer[bi];
	mavgbuffer[bi] = sample;
	mavgcount += mavgbuffer[bi];

	return mavgcount /*/ windowsize*/;
}

// END OF MOVING AVERAGE



void print_waveform(int16_t sample) {
	int x = 100 + (sample / (160));
	x = (x < 0) ? 0 : x;
	x = (x > 200) ? 200 : x;
	printf("%08d", sample);
	for (int i = 0; i < x; i++) putchar(i == 100 ? '.' : ' ');
	putchar('x');
	for (int i = x+1; i < 200; i++) putchar(i == 100 ? '.' : ' ');
	putchar('\t');
}

int bytesexpected = 0;
int bitphase = -1;

#define CRC_INIT 0x1D0F
#define CRC_POLY 0x1021
#define CRC_POST 0xFFFF
uint16_t crc = 0x1D0F;

uint16_t thecrc = 0;

void docrc(uint8_t thebyte) {
	crc = crc ^ (thebyte << 8);

	for (int i = 0; i < 8; i++) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc ^= CRC_POLY;
		} else {
			crc <<= 1;
		}
	}
}

uint8_t packet_buffer[256];
uint8_t packet_bi = 0;

void print_sanitize(uint8_t buf[], uint8_t bufi) {
	for (int i = 0; i < bufi; i++) {
		uint8_t chr = buf[i];

		if (chr >= 32) {
			putchar(chr);
		} else {
			printf("[%02X]", chr);
		}
	}
}

void process_byte(uint8_t thebyte) {	
	if (bytesexpected < 0) { //expecting length byte!
		bytesexpected = thebyte + 2; // +2 for crc
		//printf("BE: %d", bytesexpected);
		docrc(thebyte);
	} else if (bytesexpected > 0) {
		if (bytesexpected > 2) {
			packet_buffer[packet_bi++] = thebyte;
			docrc(thebyte);
		} else {
			thecrc <<= 8;
			thecrc |= thebyte;
		}
		

		bytesexpected--;
	}

	if (bytesexpected == 0) {
		crc ^= CRC_POST;
		if (crc == thecrc) {
			if (!quiet) printf("CRC OK, <");
			print_sanitize(packet_buffer, packet_bi);
			if (!quiet) printf(">");

			printf("\n");
		} else {
			if (!quiet) printf("CRC FAIL! [%02X] [%02X]\n", crc, thecrc);
		}
		
		//printf("'%.*s'", packet_bi, packet_buffer);
		bitphase = -1; // search for new preamble

		hold = false; // reset offset hold.
	}
}


int energy = 0;
int energybuf[16];
int ebi = 0;

static inline void clocktick() {
	if (debugplot) putchar('C'); 

	uint8_t thebit = (mavg > 0) ? 1 : 0; // take the sign of the moving average window. Effectively a low pass with binary threshold...

	static uint8_t thisbyte = 0;
	static uint32_t amble = 0;

	if (bitphase >= 0) {
		thisbyte = (thisbyte << 1) | (thebit & 0b1);

		bitphase++;

		if (bitphase > 7) {
			bitphase = 0;
			process_byte(thisbyte);			
		}
	}

	if (bitphase < 0) {
		amble = (amble << 1) | (thebit & 0b1);

		if ((amble & 0x00FFFFFF) == 0x00AA2D4C) { // detect 1 preamble bytes followed by 2 sync bytes
			offsethold = latestoffset;
			hold = true;

			if (!quiet) printf(">> GOT SYNC WORD, ");
			float theoffset = latestoffset * (samplerate / 32768.0) / 1000;
			if (!quiet) printf(" (OFFSET %.2fkHz) (ENERGY %.2f) ", theoffset, (float) (energy/16) * (samplerate / 32768.0) / 1000);

			packet_bi = 0;
			bitphase = 0;
			crc = CRC_INIT;

			//bitphase = -1; // temp
			bytesexpected = -1; // tell the above section to expect length byte
		}
	}

	ebi = (ebi + 1) % 16;
	energy -= energybuf[ebi];
	energybuf[ebi] = (((mavg < 0) ? -1*mavg : mavg) / windowsize);
	energy += energybuf[ebi];
}

static volatile int run = 1;

void intHandler(int dummy) {
    run = 0;
}

void mainloop() {
	#define CLKPERIOD windowsize
	int clk = 1;
	int16_t thissample = 0;
	int16_t prevsample = 0;

	while( run && rtlfm && !feof(rtlfm) ) {

		clk = (clk + 1) % CLKPERIOD;

		uint8_t b1 = fgetc(rtlfm);
		uint8_t b2 = fgetc(rtlfm);

		prevsample = thissample; // record previous sample for the purposes of zero-crossing detection
		thissample = lopass(hipass((int16_t) (b1 | (b2<<8)) /*/ 2*/)); // make headroom for filter offset. Unnecessary! filter always moves it towards zero!

		mavg = moving_average(thissample);

		if (debugplot) print_waveform(thissample);

		
		// Zero-Crossing Detector:
		if ((thissample < 0 && prevsample >= 0) || (thissample > 0 && prevsample <= 0)) {
			if (debugplot) printf("K");

			if (clk > 0 && clk < (CLKPERIOD/2)) {
				clk -= (CLKPERIOD+1); // delay clock
				//clk = clk - 1;		
				//if (clk == 0) clk = -1*CLKPERIOD;						// delay clock		
				if (debugplot) printf ("^%d", clk); 		// clock has happened recently
			} else if (clk > (CLKPERIOD/2) && clk < (CLKPERIOD)) {
				clk = (clk + 1) % CLKPERIOD;			// advance clock 
				if (debugplot) printf("v%d", clk); 		// clock is happening soon
			} else {
				if (debugplot) putchar('<');		// clock is locked on
			}		
		}

		if (clk == 0) clocktick();	


		if (debugplot) putchar('\n');

	}
}

int main (int argc, char **argv) {

	int c;

	char *helpmsg = "RTL_RFM, (C) Ryan Suchocki\n"
			"\nUsage: rtl_rfm [-hsqd] [-f freq] [-g gain] [-p error] \n\n"
			"Option flags:\n"
			"  -h    Show this message\n"
			"  -s    Read input from stdin\n"
			"  -q    Quiet. Only output good messages\n"
			"  -d    Show Debug Plot\n"
			"  -f    Frequency [869412500]\n"
			"  -g    Gain [50]\n"
			"  -p    PPM error [47]\n";

	while ((c = getopt(argc, argv, "hsqdf:g:p:")) != -1)
	switch (c)	{
		case 'h':	fprintf(stdout, "%s", helpmsg); exit(EXIT_SUCCESS); break;
		case 's':	rtlfm = stdin;										break;
		case 'q':	quiet = true;										break;
		case 'd':	debugplot = true;									break;
		case 'f':	freq = atoi(optarg);								break;
		case 'g':	gain = atoi(optarg);								break;
		case 'p':	ppm = atoi(optarg);									break;
		case '?':
		default:
			exit(EXIT_FAILURE);
	}

	signal(SIGINT, intHandler);

	init();

	hipass_init();
	lopass_init();
	moving_average_init();

	char cmdstring[128];

	sprintf(cmdstring, "rtl_fm -f %i -g %i -p %i -s %i -F 9 - %s", freq, gain, ppm, samplerate, (quiet? "2>/dev/null" : "")); /*-A fast*/

	if (!quiet) printf(">> STARTING RTL_FM ...\n\n");
	if (!rtlfm) rtlfm = popen(cmdstring, "r");

	mainloop();

	cleanup();

	if (!quiet) printf("\n>> RTL_FM FINISHED. GoodBye!\n");

	return(0);
}

