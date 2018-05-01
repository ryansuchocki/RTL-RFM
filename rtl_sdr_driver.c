#include "rtl_rfm.h"
#include "rtl_sdr_driver.h"

// Valid sample rates from the driver lib:
// 225001 - 300000 Hz
// 900001 - 3200000 Hz
// sample loss is to be expected for rates > 2400000
#define ISVALIDSAMPLERATE(x) ((x >= 225001 && x <= 300000) || (x >= 900001 && x <= 3200000))
#define DRIVER_BUFFER_SIZE (16 * 32 * 512)

int hw_init(RTLSDRInfo_t *target, int freq, int samplerate, int gain, int ppm, SampleHandler sh, bool lazy)
{
    target->downsample = 1;
    target->samplehandler = sh;
    target->lazy = lazy;
    target->acci = 0;
    target->accq = 0;
    target->count = 0;

    while (!ISVALIDSAMPLERATE(samplerate * target->downsample))
    {
        target->downsample++;

        if (samplerate * target->downsample > 2400000)
        {
            printf("NO SUITABLE DEVICE SAMPLE RATE!\n");
            return -8;
        }
    }

    printv("sample rate is %dHz, device rate is %dHz, decimating by %d\n", samplerate, samplerate * target->downsample, target->downsample);
    // check bounds?
    
    if (rtlsdr_open(&target->dev, 0) < 0) return -1;
    if (rtlsdr_set_center_freq(target->dev, freq) < 0) return -2; // Set freq before sample rate to avoid "PLL NOT LOCKED"
    if (rtlsdr_set_sample_rate(target->dev, samplerate * target->downsample) < 0) return -3;
    if (rtlsdr_set_tuner_gain_mode(target->dev, 1) < 0) return -4;
    if (rtlsdr_set_tuner_gain(target->dev, gain) < 0) return -5;
    if (ppm != 0 && rtlsdr_set_freq_correction(target->dev, ppm) < 0) return -6;
    if (rtlsdr_reset_buffer(target->dev) < 0) return -7;

    return 0;
}

void rtlsdr_callback(uint8_t *buf, uint32_t len, void *context)
{
    RTLSDRInfo_t *target = context;

    for (int i = 0; i < (int)len-1; i+=2)
    {
        target->acci += (buf[i]);
        target->accq += (buf[i+1]);

        if (target->count == target->downsample)
        {
            target->samplehandler((IQPair) {
                .i = target->acci / target->downsample - 128,
                .q = target->accq / target->downsample - 128
            }); // divide and convert to signed);

            target->count = target->acci = target->accq = 0;
        }

        target->count++;
    }
}

void rtlsdr_callback_lazy(uint8_t *buf, uint32_t len, void *context)
{
    RTLSDRInfo_t *target = context;

    for (int i = 0; i < (int)len-1; i+=2*target->downsample)
    {
        target->samplehandler((IQPair) {
            .i = (buf[i]) - 128,
            .q = (buf[i+1]) - 128
        }); // convert to signed);
    }
}

void *driver_thread_fn(void *context)
{
    RTLSDRInfo_t *target = context;

    rtlsdr_read_async(target->dev, target->lazy ? rtlsdr_callback_lazy : rtlsdr_callback, target, 0, DRIVER_BUFFER_SIZE);

    rtlsdr_close(target->dev);

    return NULL;
}

void rtl_sdr_cancel(RTLSDRInfo_t target)
{
    rtlsdr_cancel_async(target.dev);
}