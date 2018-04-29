#ifndef _FSK_H_GUARD
    #define _FSK_H_GUARD

    #include "mavg.h"

    void fsk_init(int freq, int samplerate, int baudrate);
    void fsk_cleanup();
    uint8_t fsk_decode(int16_t sample);

    Mavg hipass_filter, lopass_filter, mavg_filter;

#endif