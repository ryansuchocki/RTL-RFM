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

#include "rtl_rfm.h"
#include "hardware.h"
#include "fm.h"
#include "fsk.h"
#include "rfm_protocol.h"

#include <rtl-sdr.h>

bool quiet = false;
bool debugplot = false;
int freq = 869412500;
int gain = 50;
int ppm = 43;
int baudrate = 4800;





#define SQUELCH_THRESH 10
#define SQUELCH_NUM 16

bool squelch_state = false; // 0 is squelched, 1 is receiving
int squelch_count = 0;

bool squelch(int32_t magnitude_squared) {
	if (squelch_state) {
		if (fm_magnitude_squared < (SQUELCH_THRESH * SQUELCH_THRESH)) {
			squelch_count--;
			if(squelch_count <= 0) {
				squelch_state = false;

				fprintf(stderr, " << Carrier Lost! >>\n");
				rfm_reset();
			}
		} else {
			squelch_count = SQUELCH_NUM;
		}

		return true;
	} else {
		if (fm_magnitude_squared > (SQUELCH_THRESH * SQUELCH_THRESH)) {
			squelch_count++;
			if (squelch_count >= SQUELCH_NUM) {
				squelch_state = true;

				fprintf(stderr, " << Carrier Found! >>\n");
			}
		} else {
			squelch_count = 0;
		}

		return false;
	}
}

void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {

	for (uint32_t k = 0; k < len; k+=(DOWNSAMPLE*2)) {
		uint16_t countI = 0;
		uint16_t countQ = 0;

		for (uint32_t j = k; j < k+(DOWNSAMPLE*2); j+=2) {
			countI += ((uint8_t) buf[j]);
			countQ += ((uint8_t) buf[j+1]);
		}

		int8_t avgI = countI / DOWNSAMPLE - 128; // divide and convert to signed
		int8_t avgQ = countQ / DOWNSAMPLE - 128;

		int32_t fm_magnitude_squared = avgI * avgI + avgQ * avgQ;

		if (squelch(fm_magnitude_squared)) {
			int16_t fm = fm_demod(avgI, avgQ);
			int8_t bit = fsk_decode(fm, sqrt(fm_magnitude_squared));
			if (bit >= 0) {
				rfm_decode(bit);
			}
		}
	}
}




void intHandler(int dummy) {
    run = 0;
    rtlsdr_cancel_async(dev);
}

int main (int argc, char **argv) {

	int c;

	char *helpmsg = "RTL_RFM, (C) Ryan Suchocki\n"
			"\nUsage: rtl_rfm [-hsqd] [-f freq] [-g gain] [-p error] \n\n"
			"Option flags:\n"
			"  -h    Show this message\n"
			"  -q    Quiet. Only output good messages\n"
			"  -d    Show Debug Plot\n"
			"  -f    Frequency [869412500]\n"
			"  -g    Gain [50]\n"
			"  -p    PPM error [47]\n";

	while ((c = getopt(argc, argv, "hqdf:g:p:")) != -1)
	switch (c)	{
		case 'h':	fprintf(stdout, "%s", helpmsg); exit(EXIT_SUCCESS); break;
		case 'q':	quiet = true;										break;
		case 'd':	debugplot = true;									break;
		case 'f':	freq = atoi(optarg);								break;
		case 'g':	gain = atoi(optarg);								break;
		case 'p':	ppm = atoi(optarg);									break;
		case '?':
		default:
			exit(EXIT_FAILURE);
	}

	//signal(SIGINT, intHandler);



	if (!quiet) printf(">> STARTING RTL_FM ...\n\n");

	fsk_init();
	hw_init();

	rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, 262144);

	while (run) {

	}

	fsk_cleanup();

	if (!quiet) printf("\n>> RTL_FM FINISHED. GoodBye!\n");

	return(0);
}

