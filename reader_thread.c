#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <rtl-sdr.h>
#include <pthread.h>

#include "rtl_rfm.h"
#include "reader_thread.h"

#include "downsampler.h"
#include "fm.h"

void reader_init(void) {
    pthread_mutex_init(&data_mutex, NULL);
    pthread_cond_init(&data_cond, NULL);
   
    //memset(data,0,data_len);

    data_ready = 0;
}

void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {

    fprintf(stderr, "@");

    pthread_mutex_lock(&data_mutex);
    while (data_ready)
        pthread_cond_wait(&data_cond, &data_mutex);
    //pthread_mutex_unlock(&data_mutex);

    fprintf(stderr, "#");



    //pthread_mutex_lock(&data_mutex);

    if (len > 262144) len = 262144;
    int k = 0;

    for (uint32_t j = 0; j < len; j = j + 2) {

        int8_t i = ((uint8_t) buf[j]) - 128;
        int8_t q = ((uint8_t) buf[j+1]) - 128;

        if (downsampler(i, q)) {
            k++;
            
            int8_t di = getI();
            int8_t dq = getQ();

            int16_t fm = fm_demod(di, dq);

            data[k] = fm;
        }
    }

    data_len = k;

    fprintf(stderr, ",");

    data_ready = 1;
    fprintf(stderr, "<%i>", pthread_cond_signal(&data_cond));
    pthread_mutex_unlock(&data_mutex);
}

void *reader_entry(void *arg) {
    rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, 262144);

    return NULL;
}

void reader_start() {
    pthread_create(&reader_thread, NULL, reader_entry, NULL);
}

void reader_stop() {
    rtlsdr_cancel_async(dev);
    rtlsdr_close(dev);
}