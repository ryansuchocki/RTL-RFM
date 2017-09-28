#include "rtl_rfm.h"
#include "mavg.h"

bool mavg_init(Mavg *filter, uint16_t newsize) {
	filter->size = newsize;
	filter->data = malloc(sizeof(int16_t) * newsize);
	filter->index = 0;
	filter->count = 0;
	filter->counthold = 0;
	filter->hold = false;

	for (int i = 0; i < newsize; i++) filter->data[i] = 0;

	return true;
}

bool mavg_cleanup(Mavg *filter) {
	free(filter->data);

	return true;
}

int32_t process(Mavg *filter, int16_t sample) {

	filter->index = (filter->index + 1) % filter->size;
	filter->count += sample - filter->data[filter->index];
	filter->data[filter->index] = sample;

	return filter->count;
}

int16_t mavg_hipass(Mavg *filter, int16_t sample, bool hold) {
	if (!hold) filter->counthold = process(filter, sample);
	return sample - (filter->counthold / filter->size);
}

int16_t mavg_lopass(Mavg *filter, int16_t sample) {
	return process(filter, sample) / filter->size;
}

int32_t mavg_count(Mavg *filter, int16_t sample) {
	return process(filter, sample);
}