#include "rtl_rfm.h"
#include "fm.h"

static inline int16_t abs16(int16_t value) {
	uint16_t temp = value >> 15;     // make a mask of the sign bit
	value ^= temp;                   // toggle the bits if value is negative
	value += temp & 1;               // add one if value was negative
	return value;
}

// Linear approximation of atan2() in int16-space. Calculated in four quadrants.
// This relies on the fact that |sin(x)| - |cos(x)| is vaguely linear from x = 0 to tau/4

#define TAU INT16_MAX * 2

static inline int16_t atan2_int16(int16_t y, int16_t x) {
    int16_t absx = abs16(x);
    int16_t absy = abs16(y);

    int32_t denominator = absy + absx;
    if (denominator == 0) return 0; // avoid DBZ and skip rest of function

    int32_t numerator = (int32_t)(TAU/8) * (int32_t)(absy - absx);

    int16_t theta = ((numerator << 3) / denominator) >> 3;

    if (y >= 0) { // Note: Cartesian plane quadrants
    	if (x >= 0)	return (TAU* 1/8) + theta; // quadrant I 		Theta counts 'towards the y axis',
    	else		return (TAU* 3/8) - theta; // quadrant II 		So, negate it in quadrants II and IV
    } else {
    	if (x < 0)	return (TAU*-3/8) + theta; // quadrant III. -3/8 = 5/8
    	else		return (TAU*-1/8) - theta; // quadrant IV.  -1/8 = 7/8
    }
}

IQPair previous = {0, 0};

int16_t fm_demod(IQPair sample) {
	
	// Complex-multiply <i,q> with the conjugate of <pi,pq>
	int32_t ppr = sample.i * previous.i + sample.q * previous.q;  // May exactly overflow an int16_t (-128*-128 + -128*-128)
	int32_t ppi = sample.q * previous.i - sample.i * previous.q;

	int16_t xlp = atan2_int16(ppi, ppr);

	previous = sample;

	return xlp;
}