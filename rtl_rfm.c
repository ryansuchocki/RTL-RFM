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
#include "downsampler.h"
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

FILE *rtlstream = NULL;





rtlsdr_dev_t *dev = NULL;







int nearest_gain(rtlsdr_dev_t *dev, int target_gain)
{
	int i, r, err1, err2, count, nearest;
	int* gains;
	r = rtlsdr_set_tuner_gain_mode(dev, 1);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to enable manual gain.\n");
		return r;
	}
	count = rtlsdr_get_tuner_gains(dev, NULL);
	if (count <= 0) {
		return 0;
	}
	gains = malloc(sizeof(int) * count);
	count = rtlsdr_get_tuner_gains(dev, gains);
	nearest = gains[0];
	for (i=0; i<count; i++) {
		err1 = abs(target_gain - nearest);
		err2 = abs(target_gain - gains[i]);
		if (err2 < err1) {
			nearest = gains[i];
		}
	}
	free(gains);
	return nearest;
}

int verbose_set_frequency(rtlsdr_dev_t *dev, uint32_t frequency)
{
	int r;
	r = rtlsdr_set_center_freq(dev, frequency);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to set center freq.\n");
	} else {
		fprintf(stderr, "Tuned to %u Hz.\n", frequency);
	}
	return r;
}

int verbose_set_sample_rate(rtlsdr_dev_t *dev, uint32_t samp_rate)
{
	int r;
	r = rtlsdr_set_sample_rate(dev, samp_rate);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to set sample rate.\n");
	} else {
		fprintf(stderr, "Sampling at %u S/s.\n", samp_rate);
	}
	return r;
}

int verbose_direct_sampling(rtlsdr_dev_t *dev, int on)
{
	int r;
	r = rtlsdr_set_direct_sampling(dev, on);
	if (r != 0) {
		fprintf(stderr, "WARNING: Failed to set direct sampling mode.\n");
		return r;
	}
	if (on == 0) {
		fprintf(stderr, "Direct sampling mode disabled.\n");}
	if (on == 1) {
		fprintf(stderr, "Enabled direct sampling mode, input 1/I.\n");}
	if (on == 2) {
		fprintf(stderr, "Enabled direct sampling mode, input 2/Q.\n");}
	return r;
}

int verbose_offset_tuning(rtlsdr_dev_t *dev)
{
	int r;
	r = rtlsdr_set_offset_tuning(dev, 1);
	if (r != 0) {
		fprintf(stderr, "WARNING: Failed to set offset tuning.\n");
	} else {
		fprintf(stderr, "Offset tuning mode enabled.\n");
	}
	return r;
}

int verbose_auto_gain(rtlsdr_dev_t *dev)
{
	int r;
	r = rtlsdr_set_tuner_gain_mode(dev, 0);
	if (r != 0) {
		fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
	} else {
		fprintf(stderr, "Tuner gain set to automatic.\n");
	}
	return r;
}

int verbose_gain_set(rtlsdr_dev_t *dev, int gain)
{
	int r;
	r = rtlsdr_set_tuner_gain_mode(dev, 1);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to enable manual gain.\n");
		return r;
	}
	r = rtlsdr_set_tuner_gain(dev, gain);
	if (r != 0) {
		fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
	} else {
		fprintf(stderr, "Tuner gain set to %0.2f dB.\n", gain/10.0);
	}
	return r;
}

int verbose_ppm_set(rtlsdr_dev_t *dev, int ppm_error)
{
	int r;
	if (ppm_error == 0) {
		return 0;}
	r = rtlsdr_set_freq_correction(dev, ppm_error);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to set ppm error.\n");
	} else {
		fprintf(stderr, "Tuner error set to %i ppm.\n", ppm_error);
	}
	return r;
}

int verbose_reset_buffer(rtlsdr_dev_t *dev)
{
	int r;
	r = rtlsdr_reset_buffer(dev);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to reset buffers.\n");}
	return r;
}

int verbose_device_search(char *s)
{
	int i, device_count, device, offset;
	char *s2;
	char vendor[256], product[256], serial[256];
	device_count = rtlsdr_get_device_count();
	if (!device_count) {
		fprintf(stderr, "No supported devices found.\n");
		return -1;
	}
	fprintf(stderr, "Found %d device(s):\n", device_count);
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
	}
	fprintf(stderr, "\n");
	/* does string look like raw id number */
	device = (int)strtol(s, &s2, 0);
	if (s2[0] == '\0' && device >= 0 && device < device_count) {
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	/* does string exact match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strcmp(s, serial) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	/* does string prefix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strncmp(s, serial, strlen(s)) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	/* does string suffix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		offset = strlen(serial) - strlen(s);
		if (offset < 0) {
			continue;}
		if (strncmp(s, serial+offset, strlen(s)) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	fprintf(stderr, "No matching devices found.\n");
	return -1;
}

// vim: tabstop=8:softtabstop=8:shiftwidth=8:noexpandtab





#define SQUELCH_THRESH 10
#define SQUELCH_NUM 16

uint8_t squelch_state = 0; // 0 is squelched, 1 is receiving
int squelch_count = 0;



void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {

	int n = 0;
	int16_t countI = 0;
	int16_t countQ = 0;

	for (uint32_t j = 0; j < len; j = j + 2) {

		countI += (int8_t) (((uint8_t) buf[j]) - 128);
		countQ += (int8_t) (((uint8_t) buf[j+1]) - 128);
		n++;

		if (n == DOWNSAMPLE) {
			

			int32_t magnitude = /*sqrt*/(countI * countI + countQ * countQ); // good enough approximation?

			if (squelch_state) {
				int8_t avgI = countI / DOWNSAMPLE;
				int8_t avgQ = countQ / DOWNSAMPLE;
				int16_t fm = fm_demod(avgI, avgQ);
				int8_t bit = fsk_decode(fm, fm_magnitude);
				if (bit >= 0) {
					//fprintf(stderr, "[%i]", bit);
					rfm_decode(bit);
				}

				if (magnitude < (SQUELCH_THRESH * DOWNSAMPLE * DOWNSAMPLE * 2)) {
					squelch_count--;
					if(squelch_count <= 0) {
						squelch_state = 0;

						fprintf(stderr, " << Carrier Lost! >>\n");
						rfm_reset();
					}
				} else {
					if (squelch_count < SQUELCH_NUM) {
						squelch_count++;
					}
				}
			} else {
				if (magnitude > (SQUELCH_THRESH * DOWNSAMPLE * DOWNSAMPLE * 2)) {
					squelch_count++;
					if (squelch_count >= SQUELCH_NUM) {
						squelch_state = 1;

						fprintf(stderr, " << Carrier Found! >>\n");
					}
				} else {
					if (squelch_count > 0) {
						squelch_count--;
					}
				}
			}

			

			n = 0;
			countI = 0;
			countQ = 0;
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

	//signal(SIGINT, intHandler);

	fsk_init();

	//char cmdstring[128];

	//sprintf(cmdstring, "rtl_sdr -f %i -g %i -p %i -s %i - %s", freq, gain, ppm, BIGSAMPLERATE, (quiet? "2>/dev/null" : "")); /*-A fast*/

	if (!quiet) printf(">> STARTING RTL_FM ...\n\n");

	//if (!rtlstream) rtlstream = popen(cmdstring, "r");

	int dev_index = verbose_device_search("0");

	if ((rtlsdr_open(&dev, (uint32_t)dev_index)) < 0) {
		fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
		exit(1);
	}

	// Set freq before sample rate to avoid "PLL NOT LOCKED"
	verbose_set_frequency(dev, freq);
	verbose_set_sample_rate(dev, BIGSAMPLERATE);
	verbose_gain_set(dev, nearest_gain(dev, 500));
	verbose_ppm_set(dev, ppm);
	verbose_reset_buffer(dev);

	rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, 262144);

	while (run) {

	}

	fsk_cleanup();

	if (!quiet) printf("\n>> RTL_FM FINISHED. GoodBye!\n");

	return(0);
}

