
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
#include "fm.h"

static inline int16_t abs16(int16_t value) {
	uint16_t temp = value >> 15;     // make a mask of the sign bit
	value ^= temp;                   // toggle the bits if value is negative
	value += temp & 1;               // add one if value was negative
	return value;
}

// Linear approximation of atan2() in int16-space. Calculated in four quadrants.
// This relies on the fact that |sin(x)| - |cos(x)| is vaguely linear from x = 0 to tau/4
//         y
//         |
//     II  |  I
//  -------+------- x
//    III  |  IV
//         |
#define TAU INT16_MAX * 2
#define TURN_1_8TH TAU*1/8
#define TURN_3_8TH TAU*3/8
#define TURN_5_8TH TAU*-3/8
#define TURN_7_8TH TAU*-1/8

static inline int16_t atan2_int16(int16_t y, int16_t x) {
    const int16_t absy = abs16(y);
    const int16_t absx = abs16(x);

    int32_t denom = absy + absx;
    if (denom == 0) return 0; // avoid DBZ and skip rest of function

    int32_t theta = ((TURN_1_8TH * ((absy - absx) << 16) / denom) >> 16); 

    if (y >= 0) {
    	if (x >= 0)	return TURN_1_8TH + theta; // quadrant I 		Theta counts 'towards the y axis',
    	else		return TURN_3_8TH - theta; // quadrant II 		Therefore, negate it in quadrants II and IV
    } else {
    	if (x < 0)	return TURN_5_8TH + theta; // quadrant III
    	else		return TURN_7_8TH - theta; // quadrant IV
    }
}

int8_t pi = 0;
int8_t pq = 0;

int16_t fm_magnitude = 0;

int16_t fm_demod(int8_t i, int8_t q) {
	
	// Complex-multiply <i,q> with the conjugate of <pi,pq>
	int32_t ppr = i * pi + q * pq;  // May exactly overflow an int16_t (-128*-128 + -128*-128)
	int32_t ppi = q * pi - i * pq;

	int16_t xlp = atan2_int16(ppi, ppr);
	
	fm_magnitude = sqrt(i * i + q * q);

	pi = i;
	pq = q;

	return xlp;
}