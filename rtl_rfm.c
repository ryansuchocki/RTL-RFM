// rtl_rfm: FSK1200 Decoder
// R. Suchocki

#include "rtl_rfm.h"

#include "rb.h"
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

rb_info_t rb;
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

static inline void decode_sample(IQPair sample)
{
    if (squelch(sample, rfm_reset))
    {
        rfm_decode(fsk_decode(mavg_hipass(&hipass_filter, mavg_lopass(&lopass_filter, fm_demod(sample)))));
    }
}

volatile sig_atomic_t running = 1;

//#define THREADED

void samplehandlerfn(IQPair sample)
{
    #ifdef THREADED
    rb_put(rb, sample);
    #else
    decode_sample(sample);
    #endif
}

void main_thread_fn(void)
{
    while (running)
    {
        IQPair sample = rb_get(rb);

        #ifdef THREADED
        decode_sample(sample);
        #endif
    }
}

void intHandler(int signum)
{
    printf("\n\n>> Caught signal %d, cancelling...\n", signum);

    running = 0;
    rtl_sdr_cancel(device);
    rb_cancel(rb);
}

int main (int argc, char **argv)
{
    rb = new_rb(RESAMP_BUFFER_SIZE);

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

    float fc = samplerate * 0.443 / (8 * 16); // ~= baudrate * 0.0276; Set at 8*16 symbols so that the filter is centered after the 16-symbol sync word
    int hipass_filtersize = (0.443 * samplerate) / fc; // Number of points of mavg filter = (0.443 * Fsamplerate) / Fc

    float fc2 = baudrate * 0.5;
    int lopass_filtersize = ((float) (0.443 * (float) samplerate)) / fc2;

    lopass_filtersize = (lopass_filtersize < 1) ? 1 : lopass_filtersize;

    printv(">> Setting filters at '%.2fHz < signal < %.2fHz' (hipass size: %i, lopass size: %i)\n", fc, fc2, hipass_filtersize, lopass_filtersize);

    printv(">> RXBw is %.1fkHz around %.4fMHz.\n", (float)samplerate/1000.0, (float)freq/1000000.0);

    mavg_init(&hipass_filter, hipass_filtersize);
    mavg_init(&lopass_filter, lopass_filtersize);

    int error = hw_init(&device, freq, samplerate, gain, ppm, samplehandlerfn);

    if (error < 0)
    {
        fprintf(stderr, ">> INIT FAILED. (%d)\n", error);
        exit(EXIT_FAILURE);
    }

    printv(">> RTL_RFM READY\n\n");

    int result = 0;

    pthread_t driver_thread;

    if (pthread_create(&driver_thread, NULL, driver_thread_fn, &device))
    {
        printv(">> Error creating thread\n");
        result = EXIT_FAILURE;
    }
    else
    {
        main_thread_fn();

        if (pthread_join(driver_thread, NULL))
        {
            printv(">> Error joining driver thread\n");
            result = EXIT_FAILURE;
        }
    }

    mavg_cleanup(&hipass_filter);
    mavg_cleanup(&lopass_filter);

    fsk_cleanup();
    free_rb(rb);

    printv(">> RTL_FM FINISHED. GoodBye!\n");

    return(result);
}