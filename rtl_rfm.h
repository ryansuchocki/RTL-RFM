#ifndef _MAIN_H_GUARD
#define _MAIN_H_GUARD

// Project-wide includes
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
#include <rtl-sdr.h>

#define BIGSAMPLERATE 2457600
#define DOWNSAMPLE 64

static volatile int run = 1;

bool quiet;
bool debugplot;
int freq;
int gain;
int ppm;
int baudrate;
bool hold;
int samplerate;
int16_t latestoffset;
int16_t offsethold;

#endif