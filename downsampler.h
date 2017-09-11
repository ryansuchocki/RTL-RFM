#ifndef _DS_H_GUARD
#define _DS_H_GUARD

void downsampler_init();

bool downsampler(int8_t newreal, int8_t newimag);

int8_t getI();

int8_t getQ();

#endif