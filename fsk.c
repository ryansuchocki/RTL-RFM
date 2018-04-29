#include "rtl_rfm.h"
#include "fsk.h"

#include "mavg.h"

int windowsize;

void fsk_init(int freq, int samplerate, int baudrate) {
    windowsize = samplerate / baudrate;
    float fc = baudrate / 32; // Fc = mavg filter curoff frequency. Aim for baud/16?
    int hipass_filtersize = (0.443 * samplerate) / fc; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc

    float fc2 = baudrate * 0.5; /// 1.5;
    int lopass_filtersize = ((float) (0.443 * (float) samplerate)) / fc2;
    lopass_filtersize = (lopass_filtersize < 1) ? 1 : lopass_filtersize;

    printv(">> Setting filters at '%.2fHz < signal < %.2fHz' (hipass size: %i, lopass size: %i, window size: %i)\n", fc, fc2, hipass_filtersize, lopass_filtersize, windowsize);

    printv(">> RXBw is %.1fkHz around %.4fMHz.\n", (float)samplerate/1000.0, (float)freq/1000000.0);

    mavg_init(&hipass_filter, hipass_filtersize);
    mavg_init(&lopass_filter, lopass_filtersize);
    mavg_init(&mavg_filter, windowsize);
}

void fsk_cleanup() {
    mavg_cleanup(&hipass_filter);
    mavg_cleanup(&lopass_filter);
    mavg_cleanup(&mavg_filter);
}


void print_waveform(int16_t unfiltered, int16_t thissample, int16_t prevsample, uint8_t thebit, int clk, int32_t magnitude_squared) {
    #define SCOPEWIDTH 128
    int x = (SCOPEWIDTH/2) + ((SCOPEWIDTH/2) * unfiltered) / (INT16_MAX);
    x = (x < 0) ? 0 : x;
    x = (x > SCOPEWIDTH) ? SCOPEWIDTH : x;
    int y = (SCOPEWIDTH/2) + ((SCOPEWIDTH/2) * thissample) / (INT16_MAX);
    y = (y < 0) ? 0 : y;
    y = (y > SCOPEWIDTH) ? SCOPEWIDTH : y;
    //printf("%08d", sample);

    for (int i = 0; i < SCOPEWIDTH; i++) {
        if (i == y) putchar('X');
        else if (i == x) putchar('-');
        else if (i == (SCOPEWIDTH/2)) putchar('|');
        else putchar(' ');
    }

    // for (int i = 0; i < x; i++) putchar(i == (SCOPEWIDTH/2) ? '.' : ' ');
    // putchar('x');
    // for (int i = x+1; i < SCOPEWIDTH; i++) putchar(i == (SCOPEWIDTH/2) ? '.' : ' ');

    printf("%.2f", sqrt(magnitude_squared));
    //putchar('\t');

    if ((thissample < 0 && prevsample >= 0) || (thissample > 0 && prevsample <= 0)) {
        printf("K");

        if (clk > 0 && clk <= (windowsize/2)) {
            printf ("^%d", clk);        // clock has happened recently
        } else if (clk > (windowsize/2) && clk < (windowsize)) {
            printf("v%d", clk);         // clock is happening soon
        } else {
            putchar('<');       // clock is locked on
        }       
    }

    if (clk == 0) printf("\t%d", thebit); 

    printf("\n");
}

extern bool debugplot;
extern int32_t magnitude_squared;

uint8_t fsk_decode(int16_t sample)
{
    static int clk = 1;
    static int16_t prevsample = 0;

    clk = (clk + 1) % windowsize;

    int16_t thissample = mavg_hipass(&hipass_filter, mavg_lopass(&lopass_filter, sample));
    uint8_t thebit = (mavg_count(&mavg_filter, thissample) > 0) ? 1 : 0;

    // Zero-Crossing Detector for phase correction:
    if ((thissample < 0 && prevsample >= 0) || (thissample > 0 && prevsample <= 0)) {
        if (clk > 0 && clk <= (windowsize/2)) {
            clk -= (windowsize+1); // delay clock
        } else if (clk > (windowsize/2) && clk < (windowsize)) {
            clk = (clk + 1) % windowsize;           // advance clock 
        } // else clock locked on! Nothing to do...
    }

    if (debugplot) print_waveform(sample, thissample, prevsample, thebit, clk, magnitude_squared);

    prevsample = thissample; // record previous sample for the purposes of zero-crossing detection

    return clk ? 2 : thebit; // Process the bit when clock == 0!
}