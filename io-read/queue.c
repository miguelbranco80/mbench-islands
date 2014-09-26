#include "queue.h"

#include <stdlib.h>
#include <string.h>

void
queue_init(queue_t* queue, size_t siz, size_t maxelems)
{
    queue->nelem = 0;
    queue->maxelems = maxelems;
    queue->elemsiz = siz;
    queue->head = 0;
    queue->tail = 0;
    queue->data = malloc(siz*maxelems);
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->cond_not_full, NULL);
    pthread_cond_init(&queue->cond_not_empty, NULL);
}

void
queue_destroy(queue_t* queue)
{
    free(queue->data);
    // FIXME: destroy mutexes and conditional variables
}

void
queue_push(queue_t* queue, const void* data)
{
    pthread_mutex_lock(&queue->lock);
    while (queue->nelem == queue->maxelems) {
        pthread_cond_wait(&queue->cond_not_full, &queue->lock);
    }

    memcpy(queue->data + ( queue->head * queue->elemsiz ), data, queue->elemsiz);
    queue->head = ( queue->head + 1 ) % queue->maxelems;

    queue->nelem++;

    pthread_cond_signal(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->lock);
}

void
queue_pop(queue_t* queue, void* data)
{
    pthread_mutex_lock(&queue->lock);
    while (queue->nelem == 0) {
        pthread_cond_wait(&queue->cond_not_empty, &queue->lock);
    }
    
    memcpy(data, queue->data + ( queue->tail * queue->elemsiz ), queue->elemsiz);
    queue->tail = ( queue->tail + 1 ) % queue->maxelems;
    
    queue->nelem--;

    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->lock);
}

