#ifndef _FSK_H_GUARD
    #define _FSK_H_GUARD

    #include "mavg.h"

    void fsk_init(int samplerate, int baudrate);
    void fsk_cleanup(void);
    uint8_t fsk_decode(int16_t sample);

    Mavg mavg_filter;

#endif