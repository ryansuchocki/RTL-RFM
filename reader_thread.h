#ifndef _RT_H_GUARD
#define _RT_H_GUARD

void reader_init(void);
void reader_start();
void reader_stop();

pthread_t reader_thread;
pthread_mutex_t data_mutex;     /* Mutex to synchronize buffer access. */
pthread_cond_t data_cond;       /* Conditional variable associated. */
unsigned char data [262144];            /* Raw IQ samples buffer */
uint32_t data_len;              /* Buffer length. */

#endif