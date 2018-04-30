#include "rb.h"

rb_info_t new_rb(int s)
{
    return (rb_info_t) {
        .size = s,
        .buffer = malloc(sizeof(IQPair) * s),
        .rbi = 0,
        .rbx = 0,
        .resampled_buffer_mutex = PTHREAD_MUTEX_INITIALIZER,
        .resampled_buffer_not_empty = PTHREAD_COND_INITIALIZER,
        .resampled_buffer_not_full = PTHREAD_COND_INITIALIZER
    };
}

void free_rb(rb_info_t rb)
{
    if (rb.buffer)
        free(rb.buffer);
}

IQPair rb_get(rb_info_t rb)
{
    pthread_mutex_lock(&rb.resampled_buffer_mutex);
    while (rb.rbx == rb.rbi)
        pthread_cond_wait(&rb.resampled_buffer_not_empty, &rb.resampled_buffer_mutex);

    IQPair result = rb.buffer[rb.rbx];
    rb.rbx = (rb.rbx + 1) % rb.size;

    pthread_cond_signal(&rb.resampled_buffer_not_full);
    pthread_mutex_unlock(&rb.resampled_buffer_mutex);

    return result;
}

void rb_put(rb_info_t rb, IQPair sample)
{
    pthread_mutex_lock(&rb.resampled_buffer_mutex);
    while ((rb.rbi + 1) % rb.size == rb.rbx)
    {
        fprintf(stderr, "WARNING: WAITING ON FULL BUFFER!\n");
        pthread_cond_wait(&rb.resampled_buffer_not_full, &rb.resampled_buffer_mutex);
    }

    rb.buffer[rb.rbi] = sample;
    rb.rbi = (rb.rbi + 1) % rb.size;

    pthread_cond_signal(&rb.resampled_buffer_not_empty);
    pthread_mutex_unlock(&rb.resampled_buffer_mutex);
}