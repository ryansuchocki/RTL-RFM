#ifndef _MAVG_H_GUARD
#define _MAVG_H_GUARD

typedef struct {
	uint16_t size;
	int16_t *data;
	uint16_t index;
	int32_t count;
} Mavg;

bool mavg_init(Mavg *filter, uint16_t newsize);
bool mavg_cleanup(Mavg *filter);
int32_t process(Mavg *filter, int16_t sample);

#endif