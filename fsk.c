#include "rtl_rfm.h"
#include "fsk.h"

#include "mavg.h"

int windowsize;

void fsk_init(int samplerate, int baudrate)
{
    windowsize = samplerate / baudrate;
    mavg_init(&mavg_filter, windowsize);
}

void fsk_cleanup()
{
    mavg_cleanup(&mavg_filter);
}

#define SCOPEWIDTH 128

void print_waveform(int16_t sample, int16_t prevsample, uint8_t thebit, int clk, int32_t magnitude_squared)
{
    int y = (SCOPEWIDTH/2) + ((SCOPEWIDTH/2) * sample) / (INT16_MAX);
    y = (y < 0) ? 0 : (y > SCOPEWIDTH) ? SCOPEWIDTH : y;

    for (int i = 0; i < SCOPEWIDTH; i++)
    {
        if (i == y) putchar('X');
        else if (i == (SCOPEWIDTH/2)) putchar('|');
        else putchar(' ');
    }

    printf("%.2f", sqrt(magnitude_squared));

    if ((sample < 0 && prevsample >= 0) || (sample > 0 && prevsample <= 0))
    {
        printf("K");

        if (clk > 0 && clk <= (windowsize/2))
        {
            printf ("^%d", clk);        // clock has happened recently
        } 
        else if (clk > (windowsize/2) && clk < (windowsize))
        {
            printf("v%d", clk);         // clock is happening soon
        }
        else
        {
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

    uint8_t thebit = (mavg_count(&mavg_filter, sample) > 0) ? 1 : 0;

    // Zero-Crossing Detector for phase correction:
    if ((sample < 0 && prevsample >= 0) || (sample >= 0 && prevsample < 0))
    {
        if (clk > 0 && clk <= (windowsize/2))
        {
            clk -= (windowsize+1); // delay clock
        }
        else if (clk > (windowsize/2) && clk < (windowsize))
        {
            clk = (clk + 1) % windowsize;           // advance clock 
        } // else clock locked on! Nothing to do...
    }

    if (debugplot) print_waveform(sample, prevsample, thebit, clk, magnitude_squared);

    prevsample = sample; // record previous sample for the purposes of zero-crossing detection

    return clk ? 2 : thebit; // Process the bit when clock == 0!
}