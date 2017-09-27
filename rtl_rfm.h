#ifndef _MAIN_H_GUARD
#define _MAIN_H_GUARD

#include <rtl-sdr.h>

#define BIGSAMPLERATE 2457600
#define DOWNSAMPLE 64

static volatile int run = 1;

rtlsdr_dev_t *dev;

bool quiet;
bool debugplot;
int freq;
int gain;
int ppm;
int baudrate;

#endif