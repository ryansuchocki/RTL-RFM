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

void reader_init(void) {
    pthread_mutex_init(&data_mutex, NULL);
    pthread_cond_init(&data_cond, NULL);
   
    //memset(data,0,data_len);
}

void reader_callback(unsigned char *buf, uint32_t len, void *ctx) {
    pthread_mutex_lock(&data_mutex);

    uint32_t bytes = len;
    if (bytes > 262144) bytes = 262144;

    data_len = 0; // bytes / DOWNSAMPLE / 2;
    uint32_t j = 0;
    int16_t countI = 0;
    int16_t countQ = 0;

    for (uint32_t i=0; i<bytes; i+=2) {
        int8_t thisI = ((uint8_t) buf[i]) - 128;
        int8_t thisQ = ((uint8_t) buf[i+1]) - 128;

        countI += thisI;
        countQ += thisQ;

        j++;

        if (j == DOWNSAMPLE) {
            int16_t avgI = countI >> 5;
            int16_t avgQ = countQ >> 5;

            dataI[data_len] = (int8_t) avgI;
            dataQ[data_len] = (int8_t) avgQ;

            countI = 0;
            countQ = 0;

            j = 0;

            data_len++;
        }
    }

    //memcpy(data, buf, data_len);

    pthread_cond_signal(&data_cond);
    pthread_mutex_unlock(&data_mutex);
}

void *reader_entry(void *arg) {
    rtlsdr_read_async(dev, reader_callback, NULL, 0, 0);

    return NULL;
}

void reader_start() {
    pthread_create(&reader_thread, NULL, reader_entry, NULL);
    pthread_mutex_lock(&data_mutex);
}

void reader_stop() {
    rtlsdr_cancel_async(dev);
    rtlsdr_close(dev);
}