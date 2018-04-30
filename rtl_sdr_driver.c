#include "rtl_rfm.h"
#include "rtl_sdr_driver.h"

#define BIGSAMPLERATE 2457600
#define DRIVER_BUFFER_SIZE (16 * 32 * 512)

uint16_t downsample = 1;

SampleHandler samplehandler;
rtlsdr_dev_t *dev;

int hw_init(int freq, int samplerate, int gain, int ppm, SampleHandler sh)
{
    samplehandler = sh;

    downsample = BIGSAMPLERATE / samplerate;
    // check bounds?
    
    if (rtlsdr_open(&dev, 0) < 0) return -1;
    if (rtlsdr_set_center_freq(dev, freq) < 0) return -2; // Set freq before sample rate to avoid "PLL NOT LOCKED"
    if (rtlsdr_set_sample_rate(dev, BIGSAMPLERATE) < 0) return -3;
    if (rtlsdr_set_tuner_gain_mode(dev, 1) < 0) return -4;
    if (rtlsdr_set_tuner_gain(dev, gain) < 0) return -5;
    if (ppm != 0 && rtlsdr_set_freq_correction(dev, ppm) < 0) return -6;
    if (rtlsdr_reset_buffer(dev) < 0) return -7;

    return 0;
}

static uint32_t acci = 0, accq = 0;
static uint16_t count = 0;

void *driver_thread_fn(void *context)
{
    UNUSED(context);

    void rtlsdr_callback(uint8_t *buf, uint32_t len, void *ctx)
    {
        UNUSED(ctx);

        for (int i = 0; i < (int)len-1; i+=2)
        {
            acci += (buf[i]);
            accq += (buf[i+1]);

            if (count == downsample)
            {
                samplehandler((IQPair) {
                    .i = acci / downsample - 128,
                    .q = accq / downsample - 128
                }); // divide and convert to signed);

                count = acci = accq = 0;
            }

            count++;
        }
    }

    rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, DRIVER_BUFFER_SIZE);

    rtlsdr_close(dev);

    return NULL;
}

void rtl_sdr_cancel()
{
    rtlsdr_cancel_async(dev);
}