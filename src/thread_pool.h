// thread_pool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <worker_queue.h>
#include "http.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    pthread_t* threads;
    int num_threads;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int shutdown;

    worker_queue_t* worker_queue;
} thread_pool_t;




thread_pool_t* create_thread_pool(int num_threads, worker_queue_t* queue);

void destroy_thread_pool(thread_pool_t* pool);

#endif