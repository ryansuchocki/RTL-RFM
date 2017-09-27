#ifndef _RT_H_GUARD
#define _RT_H_GUARD

void reader_init();
void reader_start();
void reader_stop();

void reader_callback(unsigned char *buf, uint32_t len, void *ctx);

pthread_t reader_thread;
pthread_mutex_t data_mutex;     /* Mutex to synchronize buffer access. */
pthread_cond_t data_cond;       /* Conditional variable associated. */
int data_ready;
int8_t data[2048];            
uint16_t data_len;              /* Buffer length. */

#endif