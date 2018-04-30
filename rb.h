#ifndef _RB_H_GUARD
    #define _RB_H_GUARD

	#include "rtl_rfm.h"

	typedef struct RB_INFO_s
	{
		int size;
		IQPair *buffer;
		uint16_t *rb_in;
		uint16_t *rb_out;
		pthread_mutex_t *resampled_buffer_mutex; // need to init these
		pthread_cond_t *resampled_buffer_not_empty;
		pthread_cond_t *resampled_buffer_not_full;
	} rb_info_t;

	rb_info_t new_rb(int s);
	void free_rb(rb_info_t rb);
	void rb_cancel(rb_info_t rb);
	IQPair rb_get(rb_info_t rb);
	void rb_put(rb_info_t rb, IQPair sample);
#endif