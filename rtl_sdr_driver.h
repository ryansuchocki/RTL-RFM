#ifndef _RTLSDRDRIVER_H_GUARD
    #define _RTLSDRDRIVER_H_GUARD

    #include <rtl-sdr.h>

	typedef void (*SampleHandler)(IQPair sample);

	int hw_init(int freq, int samplerate, int gain, int ppm, SampleHandler sh);
	void *driver_thread_fn(void *context);
	void rtl_sdr_cancel();
	
#endif