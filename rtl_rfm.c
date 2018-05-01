// rtl_rfm: FSK1200 Decoder
// R. Suchocki

#include "rtl_rfm.h"

#include "rtl_sdr_driver.h"
#include "squelch.h"
#include "fm.h"
#include "fsk.h"
#include "rfm_protocol.h"

#define RESAMP_BUFFER_SIZE 2048 * 32 // 2048 resampled in one burst from driver

bool quiet = false;
bool debugplot = false;
int freq = 869412500;
int gain = 496; // 49.6
int ppm = 0;
int baudrate = 4800;
int samplerate = 38400;
bool lazydecimate = false;

RTLSDRInfo_t device;

Mavg hipass_filter, lopass_filter;

void printv(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if(!quiet)
        vprintf(format, args);

    va_end(args);
}

void try_free(void *it)
{
    if (it) free(it);
}

char *print_sanitize(char* buf)
{
    if (buf)
    {
        for (unsigned int i = 0; i < strlen(buf); i++)
        {
            uint8_t chr = buf[i];

            if (chr >= 32)
            {
                putchar(chr);
            }
            else
            {
                printf("[%02X]", chr);
            }
        }
    }
    
    return buf;
}

void filter_reset(void)
{
    hipass_filter.hold = false;    // reset offset hold.
}

void filter_hold(void)
{
    hipass_filter.hold = true;
    printv(">> GOT SYNC WORD, ");
    float foffset = (float) hipass_filter.counthold * (float) samplerate / (float) INT16_MAX / (float) hipass_filter.size / 2.0;
    float eppm = 1000000.0 * foffset / freq;
    printv("(OFFSET %.0fHz = %.1f PPM)", foffset, eppm);
}

void squelch_close_cb(void)
{
    rfm_reset();
    filter_reset();
}

static inline int16_t bandpass(int16_t sample)
{
    return mavg_hipass(&hipass_filter, mavg_lopass(&lopass_filter, sample));
}

static inline IQPair channelize(IQPair sample)
{
    float lo_f = 0; // in Hz
    static int t = 0;
    t++;

    IQPair LO = {
        .i = INT8_MAX * cos(2 * M_PI * lo_f/samplerate * (float) t),
        .q = INT8_MAX * sin(2 * M_PI * lo_f/samplerate * (float) t)
    };

    //IQPair filteredsample = mavg_lopass(&channel_filter, mixed);
    // need to apply filter to I and Q seperately. And decimate?

    return IQPAIR_PRODUCT(sample, LO);
}

void samplehandlerfn(IQPair sample)
{
    IQPair channelsample = sample; //channelize(sample);

    if (squelch(channelsample, squelch_close_cb))
    {
        try_free(
            print_sanitize(
                rfm_decode(
                    fsk_decode(
                        bandpass(
                            fm_demod(
                                channelsample))))));
    }
}

void intHandler(int signum)
{
    printf("\n\n>> Caught signal %d, cancelling...\n", signum);

    rtl_sdr_cancel(device);
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
        "  -p    PPM error\n"
        "  -l    Lazy Decimation\n";

    int c;

    while ((c = getopt(argc, argv, "hqdlf:g:p:")) != -1)
    {
        switch (c)
        {
            case 'h':   fprintf(stdout, "%s", helpmsg); exit(EXIT_SUCCESS); break;
            case 'q':   quiet = true;                                       break;
            case 'd':   debugplot = true;                                   break;
            case 'l':   lazydecimate = true;                                break;
            case 'f':   freq = atoi(optarg);                                break;
            case 'g':   gain = atof(optarg) * 10;                           break;
            case 'p':   ppm = atoi(optarg);                                 break;
            case '?':
            default: exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, intHandler);

    printv(">> STARTING RTL_RFM ...\n");

    float fc = samplerate * 0.443 / (8 * 16); // ~= baudrate * 0.0276; Set at 8*16 symbols so that the filter is centered after the 16-symbol sync word
    int hipass_filtersize = (0.443 * samplerate) / fc; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc

    float fc2 = baudrate * 0.5;
    int lopass_filtersize = ((float) (0.443 * (float) samplerate)) / fc2;

    lopass_filtersize = (lopass_filtersize < 1) ? 1 : lopass_filtersize;

    printv(">> Setting filters at '%.2fHz < signal < %.2fHz' (hipass size: %i, lopass size: %i)\n", fc, fc2, hipass_filtersize, lopass_filtersize);

    printv(">> RXBw is %.1fkHz around %.4fMHz.\n", (float)samplerate/1000.0, (float)freq/1000000.0);

    mavg_init(&hipass_filter, hipass_filtersize);
    mavg_init(&lopass_filter, lopass_filtersize);

    rfm_init(filter_hold, filter_reset);
    fsk_init(samplerate, baudrate);

    int error = hw_init(&device, freq, samplerate, gain, ppm, samplehandlerfn, lazydecimate);

    if (error < 0)
    {
        fprintf(stderr, ">> INIT FAILED. (%d)\n", error);
        exit(EXIT_FAILURE);
    }

    printv(">> RTL_RFM READY\n\n");

    int result = 0;

    driver_thread_fn(&device);

    mavg_cleanup(&hipass_filter);
    mavg_cleanup(&lopass_filter);

    fsk_cleanup();

    printv(">> RTL_FM FINISHED. GoodBye!\n");

    return(result);
}