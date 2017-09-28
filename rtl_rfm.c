// rtl_rfm: FSK1200 Decoder
// R. Suchocki

#include "rtl_rfm.h"

#include "squelch.h"
#include "fm.h"
#include "fsk.h"
#include "rfm_protocol.h"


#define BIGSAMPLERATE 2457600
#define DOWNSAMPLE 64

bool quiet = false;
bool debugplot = false;
int freq = 869412500;
int gain = 496; // 49.6
int ppm = 43;
int baudrate = 4800;
int samplerate = BIGSAMPLERATE/DOWNSAMPLE;/*31200;*/ /*19200;*/ //38400; // multiple of baudrate

rtlsdr_dev_t *dev = NULL;

int hw_init() {
	if (rtlsdr_open(&dev, 0) < 0) return -1;
	if (rtlsdr_set_center_freq(dev, freq) < 0) return -2; // Set freq before sample rate to avoid "PLL NOT LOCKED"
	if (rtlsdr_set_sample_rate(dev, BIGSAMPLERATE) < 0) return -3;
	if (rtlsdr_set_tuner_gain_mode(dev, 1) < 0) return -4;
	if (rtlsdr_set_tuner_gain(dev, gain) < 0) return -5;
	if (rtlsdr_set_freq_correction(dev, ppm) < 0) return -6;
	if (rtlsdr_reset_buffer(dev) < 0) return -7;

	return 0;
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
				rfm_decode(bit, samplerate);
			}
		}
	}
}

void intHandler(int signal) {
    rtlsdr_cancel_async(dev);
}

int main (int argc, char **argv) {
	char *helpmsg = "RTL_RFM, (C) Ryan Suchocki\n"
		"\nUsage: rtl_rfm [-hsqd] [-f freq] [-g gain] [-p error] \n\n"
		"Option flags:\n"
		"  -h    Show this message\n"
		"  -q    Quiet. Only output good messages\n"
		"  -d    Show Debug Plot\n"
		"  -f    Frequency [869412500]\n"
		"  -g    Gain [49.6]\n"
		"  -p    PPM error [43]\n";

	int c;

	while ((c = getopt(argc, argv, "hqdf:g:p:")) != -1) {
		switch (c)	{
			case 'h':	fprintf(stdout, "%s", helpmsg); exit(EXIT_SUCCESS); break;
			case 'q':	quiet = true;										break;
			case 'd':	debugplot = true;									break;
			case 'f':	freq = atoi(optarg);								break;
			case 'g':	gain = atof(optarg) * 10;							break;
			case 'p':	ppm = atoi(optarg);									break;
			case '?':
			default: exit(EXIT_FAILURE);
		}
	}

	signal(SIGINT, intHandler);

	if (!quiet) printf(">> STARTING RTL_RFM ...\n");

	fsk_init(freq, samplerate, baudrate);

	int error = hw_init();
	if (error >= 0) {
		if (!quiet) printf(">> RTL_RFM READY\n\n");
	} else {
		fprintf(stderr, ">> INIT FAILED. (%d)", error);
		exit(EXIT_FAILURE);
	}

	rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, 262144);

	fsk_cleanup();

	if (!quiet) printf("\n>> RTL_FM FINISHED. GoodBye!\n");

	return(0);
}