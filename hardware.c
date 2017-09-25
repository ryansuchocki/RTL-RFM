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

#include "rtl_rfm.h"
#include "hardware.h"

int hardware_init() {
    int j;
    int dev_index = 0;
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
