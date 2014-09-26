#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h>

typedef struct queue_s {
    pthread_mutex_t lock;
    pthread_cond_t  cond_not_full;
    pthread_cond_t  cond_not_empty;
    
    unsigned char*  data;
    size_t          elemsiz;

    size_t          nelem;
    size_t          maxelems;
        
    size_t          head;
    size_t          tail;    
} queue_t;


void queue_init(queue_t* queue, size_t siz, size_t maxelems);
void queue_destroy(queue_t* queue);
void queue_push(queue_t* queue, const void* data);
void queue_pop(queue_t* queue, void* data);

#endif /* QUEUE_H_ */
