#ifndef _MAIN_H_GUARD
#define _MAIN_H_GUARD

#define BIGSAMPLERATE 2457600
#define DOWNSAMPLE 128

static volatile int run = 1;

bool quiet;
bool debugplot;
int freq;
int gain;
int ppm;
int baudrate;

#endif