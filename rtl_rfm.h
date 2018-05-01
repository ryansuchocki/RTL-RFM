#ifndef _MAIN_H_GUARD
    #define _MAIN_H_GUARD

    // Project-wide helper macros:

    #define UNUSED(x) (void)(x)

    // Project-wide includes:

        // C Standard Library (POSIX)
        #include <unistd.h>
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
        #include <stdarg.h>

        #include "IQ16.h"

    // Project-wide globals:

    void printv(const char *format, ...);

    typedef void (*CB_VOID)(void);

#endif