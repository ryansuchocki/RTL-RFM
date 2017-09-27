#include "rtl_rfm.h"
#include "hardware.h"

rtlsdr_dev_t *dev = NULL;

int hw_init() {
	if (rtlsdr_open(&dev, 0) < 0) return -1;
	if (rtlsdr_set_center_freq(dev, freq) < 0) return -2; // Set freq before sample rate to avoid "PLL NOT LOCKED"
	if (rtlsdr_set_sample_rate(dev, BIGSAMPLERATE) < 0) return -3;
	if (rtlsdr_set_tuner_gain_mode(dev, 1) < 0) return -4;
	if (rtlsdr_set_tuner_gain(dev, gain) < 0) return -5;
	if (rtlsdr_set_freq_correction(dev, ppm) < 0) return -6;
	if (rtlsdr_reset_buffer(dev) < 0) return -7;

	if (!quiet) printf(">> RTL_RFM READY\n\n");


	return 0;
}