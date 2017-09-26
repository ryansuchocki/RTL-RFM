
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "rtl_rfm.h"
#include "downsampler.h"


int8_t ibuf[DOWNSAMPLE];
int8_t qbuf[DOWNSAMPLE];
uint8_t bufi = 0;
int16_t icount = 0;
int16_t qcount = 0;

void downsampler_init() {
	for (int i = 0; i < DOWNSAMPLE; i++) {
		ibuf[i] = 0;
		qbuf[i] = 0;
	}
}

bool downsampler(int8_t newi, int8_t newq) {
	if (bufi == 0) {
		icount = 0;
		qcount = 0;
	}
	
	icount += newi;
	qcount += newq;

	bufi = (bufi + 1) % DOWNSAMPLE;

	return (bufi == 0);
}

int8_t getI() {
	return icount / DOWNSAMPLE;
}

int8_t getQ() {
	return qcount / DOWNSAMPLE;
}
