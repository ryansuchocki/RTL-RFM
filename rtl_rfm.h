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

    // Project-wide globals can go here:
    typedef struct IQPair {
        int16_t i;
        int16_t q;
    } IQPair;
#endif