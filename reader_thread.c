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

    data_ready = 0;
}


void *reader_entry(void *arg) {
    rtlsdr_read_async(dev, reader_callback, NULL, 0, 512);

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