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

#include <rtl-sdr.h>
#include <pthread.h>

#include "rtl_rfm.h"
#include "hardware.h"
#include "reader_thread.h"
#include "downsampler.h"
#include "fm.h"
#include "fsk.h"
#include "rfm_protocol.h"

bool quiet = false;
int freq = 869412500;
//int gain = 50;
int ppm = 43;
int baudrate = 4800;

int gain = 490;


static void sighandler(int signum) {
	if (signum == SIGINT) {
		fprintf(stderr, "\n>> Received SIGINT\n");
		run = 0;
	}    
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

	signal(SIGINT, sighandler);

	hardware_init();

	fsk_init();

	reader_start();

	while(run) {
        pthread_cond_wait(&data_cond, &data_mutex);

        for (uint32_t i = 0; i < data_len; i++) {

        	//fprintf(stderr, "got %i.\n", data_len);

        	int8_t di = dataI[i] / DOWNSAMPLE;
			int8_t dq = dataQ[i] / DOWNSAMPLE;

			int16_t fm = fm_demod(di, dq);

			int8_t bit = fsk_decode(fm, fm_magnitude);
			if (bit >= 0) rfm_decode(bit);
        }
    }

    if (!quiet) printf("\n>> RTL_RFM Shutting Down!\n");

    reader_stop();

	fsk_cleanup();

	if (!quiet) printf("\n>> RTL_RFM FINISHED. GoodBye!\n");

	return(0);
}

