#ifndef _FSK_H_GUARD
#define _FSK_H_GUARD

#include "mavg.h"

void fsk_init(int freq, int samplerate, int baudrate);
void fsk_cleanup();
int8_t fsk_decode(int16_t sample, int16_t magnitude);

Mavg filter, filter2, filter3;

#endif