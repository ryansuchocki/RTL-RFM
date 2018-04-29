// rtl_rfm: FSK1200 Decoder
// R. Suchocki

#include "rtl_rfm.h"

#include "squelch.h"
#include "fm.h"
#include "fsk.h"
#include "rfm_protocol.h"

#define BIGSAMPLERATE 2457600
#define DOWNSAMPLE 64 // CANNOT BE MORE THAN 128!!

#define DRIVER_BUFFER_SIZE 16 * 32 * 512
#define RESAMP_BUFFER_SIZE DRIVER_BUFFER_SIZE / DOWNSAMPLE / 2

bool quiet = false;
bool debugplot = false;
int freq = 869412500;
int gain = 496; // 49.6
int ppm = 0;
int baudrate = 4800;
int samplerate = BIGSAMPLERATE/DOWNSAMPLE;/*31200;*/ /*19200;*/ //38400; // multiple of baudrate

rtlsdr_dev_t *dev = NULL;

int hw_init()
{
    //float freq_corr = (float)freq * (1 - ppm / 1000000.0);

    //printv("Corrected Frequency: %.0f FHz", freq_corr);

    if (rtlsdr_open(&dev, 0) < 0) return -1;
    if (rtlsdr_set_center_freq(dev, freq) < 0) return -2; // Set freq before sample rate to avoid "PLL NOT LOCKED"
    if (rtlsdr_set_sample_rate(dev, BIGSAMPLERATE) < 0) return -3;
    if (rtlsdr_set_tuner_gain_mode(dev, 1) < 0) return -4;
    if (rtlsdr_set_tuner_gain(dev, gain) < 0) return -5;
    if (ppm != 0 && rtlsdr_set_freq_correction(dev, ppm) < 0) return -6;
    //rtlsdr_set_freq_correction(dev, 0);
    if (rtlsdr_reset_buffer(dev) < 0) return -7;

    return 0;
}

IQPair resampled_buffer[RESAMP_BUFFER_SIZE];
uint16_t rbi = 0;

void rtlsdr_callback(uint8_t *buf, uint32_t len, void *ctx) {
    UNUSED(ctx);

    uint32_t acci = 0, accq = 0;

    for (int i = 0, count = 0; i < (int)len-1; i+=2, count++)
    {
        acci += (buf[i]);
        accq += (buf[i+1]);

        if (count == DOWNSAMPLE)
        {
            resampled_buffer[rbi++] = (IQPair)
            {
                .i = acci / DOWNSAMPLE - 128,
                .q = accq / DOWNSAMPLE - 128
            }; // divide and convert to signed
            count = acci = accq = 0;
        }
    }

    for (int i = 0; i < rbi; i++)
    {
        if (squelch(resampled_buffer[i], rfm_reset))
        {
            rfm_decode(fsk_decode(fm_demod(resampled_buffer[i])));
        }
    }

    rbi = 0;
}

void intHandler(int signum)
{
    printf(">> Caught signal %d, cancelling...", signum);

    rtlsdr_cancel_async(dev);
}

void printv(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if(!quiet)
        vprintf(format, args);

    va_end(args);
}

int main (int argc, char **argv)
{
    char *helpmsg = "RTL_RFM, (C) Ryan Suchocki\n"
        "\nUsage: rtl_rfm [-hsqd] [-f freq] [-g gain] [-p error] \n\n"
        "Option flags:\n"
        "  -h    Show this message\n"
        "  -q    Quiet. Only output good messages\n"
        "  -d    Show Debug Plot\n"
        "  -f    Frequency [869412500]\n"
        "  -g    Gain [49.6]\n"
        "  -p    PPM error\n";

    int c;

    while ((c = getopt(argc, argv, "hqdf:g:p:")) != -1)
    {
        switch (c)
        {
            case 'h':   fprintf(stdout, "%s", helpmsg); exit(EXIT_SUCCESS); break;
            case 'q':   quiet = true;                                       break;
            case 'd':   debugplot = true;                                   break;
            case 'f':   freq = atoi(optarg);                                break;
            case 'g':   gain = atof(optarg) * 10;                           break;
            case 'p':   ppm = atoi(optarg);                                 break;
            case '?':
            default: exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, intHandler);

    printv(">> STARTING RTL_RFM ...\n");

    fsk_init(freq, samplerate, baudrate);

    int error = hw_init();

    if (error >= 0)
    {
        printv(">> RTL_RFM READY\n\n");
    }
    else
    {
        fprintf(stderr, ">> INIT FAILED. (%d)\n", error);
        exit(EXIT_FAILURE);
    }

    rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, DRIVER_BUFFER_SIZE);

    rtlsdr_close(dev);

    fsk_cleanup();

    printv("\n>> RTL_FM FINISHED. GoodBye!\n");

    return(0);
}