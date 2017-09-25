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
#include "downsampler.h"
#include "fm.h"
#include "fsk.h"
#include "rfm_protocol.h"

bool quiet = false;
bool debugplot = false;
int freq = 869412500;
//int gain = 50;
int ppm = 43;
int baudrate = 4800;

FILE *rtlstream = NULL;


int dev_index;
rtlsdr_dev_t *dev;
int gain = 490;
static int setup_hardware() {
    int j;
    int device_count;
    char vendor[256], product[256], serial[256];

    device_count = rtlsdr_get_device_count();
    if (!device_count) {
        fprintf(stderr, "No supported RTLSDR devices found.\n");
        exit(1);
    }

    fprintf(stderr, "Found %d device(s):\n", device_count);
    for (j = 0; j < device_count; j++) {
        rtlsdr_get_device_usb_strings(j, vendor, product, serial);
        fprintf(stderr, "%d: %s, %s, SN: %s %s\n", j, vendor, product, serial,
            (j == dev_index) ? "(currently selected)" : "");
    }

    if (rtlsdr_open(&dev, dev_index) < 0) {
        fprintf(stderr, "Error opening the RTLSDR device: %s\n",
            strerror(errno));
        exit(1);
    }

    /* Set gain, frequency, sample rate, and reset the device. */
    rtlsdr_set_tuner_gain_mode(dev, (gain == -100) ? 0 : 1);
    if (gain != -100) {
        if (gain == 999999) {
            /* Find the maximum gain available. */
            int numgains;
            int gains[100];

            numgains = rtlsdr_get_tuner_gains(dev, gains);
            gain = gains[numgains-1];
            fprintf(stderr, "Max available gain is: %.2f\n", gain/10.0);
        }
        rtlsdr_set_tuner_gain(dev, gain);
        fprintf(stderr, "Setting gain to: %.2f\n", gain/10.0);
    } else {
        fprintf(stderr, "Using automatic gain control.\n");
    }
    rtlsdr_set_freq_correction(dev, ppm);
    //if (Modes.enable_agc) rtlsdr_set_agc_mode(dev, 1);
    rtlsdr_set_center_freq(dev, freq);
    rtlsdr_set_sample_rate(dev, BIGSAMPLERATE);
    rtlsdr_reset_buffer(dev);
    fprintf(stderr, "Gain reported by device: %.2f\n",
        rtlsdr_get_tuner_gain(dev)/10.0);

    return 0;
}

pthread_t reader_thread;
pthread_mutex_t data_mutex;     /* Mutex to synchronize buffer access. */
pthread_cond_t data_cond;       /* Conditional variable associated. */
unsigned char data [262144];            /* Raw IQ samples buffer */
uint32_t data_len;              /* Buffer length. */

void reader_init(void) {

    pthread_mutex_init(&data_mutex, NULL);
    pthread_cond_init(&data_cond, NULL);
   
    //memset(data,0,data_len);
}


void rtlsdrCallback(unsigned char *buf, uint32_t len, void *ctx) {
    pthread_mutex_lock(&data_mutex);

    //fprintf(stderr, "Got %i bytes!", len);

    data_len = len;
    if (data_len > 262144) data_len = 262144;

    memcpy(data, buf, data_len);

    pthread_cond_signal(&data_cond);
    pthread_mutex_unlock(&data_mutex);
}

void *readerThreadEntryPoint(void *arg) {
    rtlsdr_read_async(dev, rtlsdrCallback, NULL, 0, 0);

    return NULL;
}





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
			"  -s    Read input from stdin\n"
			"  -q    Quiet. Only output good messages\n"
			"  -d    Show Debug Plot\n"
			"  -f    Frequency [869412500]\n"
			"  -g    Gain [50]\n"
			"  -p    PPM error [47]\n";

	while ((c = getopt(argc, argv, "hsqdf:g:p:")) != -1)
	switch (c)	{
		case 'h':	fprintf(stdout, "%s", helpmsg); exit(EXIT_SUCCESS); break;
		case 's':	rtlstream = stdin;										break;
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

	setup_hardware();

	fsk_init();

	pthread_create(&reader_thread, NULL, readerThreadEntryPoint, NULL);

	pthread_mutex_lock(&data_mutex);

	while(run) {
        pthread_cond_wait(&data_cond, &data_mutex);

        //pthread_mutex_lock(&data_mutex);

        //pthread_mutex_unlock(&data_mutex);

        //fprintf(stderr, "BAMBOOZLE!");

        // do something with the data

        for (uint32_t j = 0; j < data_len; j+=2) {

        	int8_t i = ((uint8_t) data[j]) - 128;
			int8_t q = ((uint8_t) data[j+1]) - 128;

			if (downsampler(i, q)) {
				int8_t di = getI();
				int8_t dq = getQ();

				int16_t fm = fm_demod(di, dq);

				int8_t bit = fsk_decode(fm, fm_magnitude);
				if (bit >= 0) rfm_decode(bit);
			}
        }

        //pthread_mutex_lock(&data_mutex);
    }

    fprintf(stderr, "\n>> Shutting Down!\n");

    rtlsdr_cancel_async(dev);
	rtlsdr_close(dev);

	fsk_cleanup();









	//fsk_init();

	//char cmdstring[128];

	// sprintf(cmdstring, "rtl_sdr -f %i -g %i -p %i -s %i - %s", freq, gain, ppm, BIGSAMPLERATE, (quiet? "2>/dev/null" : "")); /*-A fast*/

	// if (!quiet) printf(">> STARTING RTL_FM ...\n\n");

	// if (!rtlstream) rtlstream = popen(cmdstring, "r");

	// if (!rtlstream) {
	// 	printf("\n>> ERROR\n");
	// } else {
	// 	while( run && !feof(rtlstream) ) {

	// 		int8_t i = ((uint8_t) fgetc(rtlstream)) - 128;
	// 		int8_t q = ((uint8_t) fgetc(rtlstream)) - 128;

	// 		if (downsampler(i, q)) {
	// 			int8_t di = getI();
	// 			int8_t dq = getQ();

	// 			int16_t fm = fm_demod(di, dq);

	// 			int8_t bit = fsk_decode(fm, fm_magnitude);
	// 			if (bit >= 0) rfm_decode(bit);
	// 		}
	// 	}
	// }

	//fsk_cleanup();

	if (!quiet) printf("\n>> RTL_FM FINISHED. GoodBye!\n");

	return(0);
}

