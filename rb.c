#include "rb.h"

rb_info_t new_rb(int s)
{
    pthread_mutex_t *m = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m, NULL);

    pthread_cond_t *cne = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(cne, NULL);

    pthread_cond_t *cnf = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(cnf, NULL);

    uint16_t *in = malloc(sizeof(uint16_t));
    *in = 0;

    uint16_t *out = malloc(sizeof(uint16_t));
    *out = 0;

    return (rb_info_t) {
        .size = s,
        .buffer = malloc(sizeof(IQPair) * s),
        .rb_in = in,
        .rb_out = out,
        .resampled_buffer_mutex = m,
        .resampled_buffer_not_empty = cne,
        .resampled_buffer_not_full = cnf
    };
}

void free_rb(rb_info_t rb)
{
    if (rb.resampled_buffer_mutex)
        free(rb.resampled_buffer_mutex);

    if (rb.resampled_buffer_not_empty)
        free(rb.resampled_buffer_not_empty);

    if (rb.resampled_buffer_not_full)
        free(rb.resampled_buffer_not_full);

    if (rb.rb_in)
        free(rb.rb_in);

    if (rb.rb_out)
        free(rb.rb_out);

    if (rb.buffer)
        free(rb.buffer);
}

IQPair rb_get(rb_info_t rb)
{
    pthread_mutex_lock(rb.resampled_buffer_mutex);
    while (*rb.rb_out == *rb.rb_in)
    {
        pthread_cond_wait(rb.resampled_buffer_not_empty, rb.resampled_buffer_mutex);
    }

    IQPair result = rb.buffer[*rb.rb_out];
    *rb.rb_out = (*rb.rb_out + 1) % rb.size;

    pthread_cond_signal(rb.resampled_buffer_not_full);
    pthread_mutex_unlock(rb.resampled_buffer_mutex);

    return result;
}

void rb_put(rb_info_t rb, IQPair sample)
{
    pthread_mutex_lock(rb.resampled_buffer_mutex);
    while ((*rb.rb_in + 1) % rb.size == *rb.rb_out)
    {
        fprintf(stderr, "WARNING: WAITING ON FULL BUFFER!\n");
        pthread_cond_wait(rb.resampled_buffer_not_full, rb.resampled_buffer_mutex);
    }

    rb.buffer[*rb.rb_in] = sample;
    *rb.rb_in = (*rb.rb_in + 1) % rb.size;

    pthread_cond_signal(rb.resampled_buffer_not_empty);
    pthread_mutex_unlock(rb.resampled_buffer_mutex);
}

void rb_cancel(rb_info_t rb)
{
    pthread_mutex_lock(rb.resampled_buffer_mutex);
    pthread_cond_signal(rb.resampled_buffer_not_full);
    pthread_cond_signal(rb.resampled_buffer_not_empty);
    pthread_mutex_unlock(rb.resampled_buffer_mutex);
}