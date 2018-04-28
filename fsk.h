#ifndef _FSK_H_GUARD
	#define _FSK_H_GUARD

	#include "mavg.h"

	void fsk_init(int freq, int samplerate, int baudrate, bool quiet);
	void fsk_cleanup();
	int8_t fsk_decode(int16_t sample, int32_t magnitude_squared, bool debugplot);

	Mavg hipassfilter, lopassfilter, windowfilter;

#endif