#include "rtl_rfm.h"
#include "hardware.h"

rtlsdr_dev_t *dev = NULL;

int nearest_gain(rtlsdr_dev_t *dev, int target_gain) {
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

int hw_init() {
	if (rtlsdr_open(&dev, 0) < 0) return -1;
	if (rtlsdr_set_center_freq(dev, freq) < 0) return -2; // Set freq before sample rate to avoid "PLL NOT LOCKED"
	if (rtlsdr_set_sample_rate(dev, BIGSAMPLERATE) < 0) return -3;
	if (rtlsdr_set_tuner_gain_mode(dev, 1) < 0) return -4;
	if (rtlsdr_set_tuner_gain(dev, nearest_gain(dev, 500)) < 0) return -5;
	if (rtlsdr_set_freq_correction(dev, ppm_error) < 0) return -6;
	if (rtlsdr_reset_buffer(dev) < 0) return -7;

	return 0;
}