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

int gain = /*999999;*/ 496;


static void sighandler(int signum) {
	if (signum == SIGINT) {
		reader_stop();

		
		fprintf(stderr, "\n>> Received SIGINT\n");
		run = 0;
	}    
}


void reader_callback(unsigned char *buf, uint32_t len, void *ctx) {

    uint32_t bytes = len;
    //if (bytes > 262144) bytes = 262144;

    fprintf(stderr, "%i\t", len);

    data_len = 0; // bytes / DOWNSAMPLE / 2;
    uint32_t j = 0;
    int16_t countI = 0;
    int16_t countQ = 0;

    for (uint32_t i=0; i<bytes; i+=2) {
        int8_t thisI = ((uint8_t) buf[i]) - 128;
        int8_t thisQ = ((uint8_t) buf[i+1]) - 128;

        countI += thisI;
        countQ += thisQ;

        j++;

        /*if (j == DOWNSAMPLE) {
            int16_t avgI = countI / DOWNSAMPLE;
            int16_t avgQ = countQ / DOWNSAMPLE;

            int8_t bit = fsk_decode(fm_demod(avgI, avgQ), fm_magnitude);
            if (bit >= 0) rfm_decode(bit);

            //dataI[data_len] = (int8_t) avgI;
            //dataQ[data_len] = (int8_t) avgQ;

            countI = 0;
            countQ = 0;

            j = 0;

            data_len++;
        }*/
    }

    //memcpy(data, buf, data_len);

    data_ready = true;
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

	//reader_start();

	rtlsdr_read_async(dev, reader_callback, NULL, 0, 0);

	/*while(run) {
		pthread_mutex_lock(&data_mutex);
		while (data_ready == 0)
		    pthread_cond_wait(&data_cond, &data_mutex);
		

        for (uint16_t i = 0; i < data_len; i++) {

			int8_t bit = fsk_decode(fm_demod(dataI[i], dataQ[i]), fm_magnitude);
			if (bit >= 0) rfm_decode(bit);
        }

        data_ready = false;

        pthread_mutex_unlock(&data_mutex);
    }*/

    if (!quiet) printf("\n>> RTL_RFM Shutting Down!\n");

    reader_stop();

	fsk_cleanup();

	if (!quiet) printf("\n>> RTL_RFM FINISHED. GoodBye!\n");

	return(0);
}

