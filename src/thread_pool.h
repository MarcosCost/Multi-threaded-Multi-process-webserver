// thread_pool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "worker_queue.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "cache.h"

typedef struct {
    pthread_t* threads;
    int num_threads;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int shutdown;

    worker_queue_t* worker_queue;
    shared_memory_t * shm;
    semaphores_t * sems;
    cache_t * cache;

} thread_pool_t;


thread_pool_t* create_thread_pool(int num_threads, worker_queue_t* queue, shared_memory_t * shm, semaphores_t * sems);

void destroy_thread_pool(thread_pool_t* pool);

#endif