#ifndef STATS_H
#define STATS_H


#include "shared_mem.h"
#include "semaphores.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>



void * start_stats(void *arg);

void add_connection(shared_memory_t * shm, semaphores_t * sems);
void remove_connection(shared_memory_t * shm, semaphores_t * sems);

void add_request_total(shared_memory_t * shm, semaphores_t * sems);
void add_bytes_transferred(shared_memory_t * shm, semaphores_t * sems, int bytes);
void add_status_code(shared_memory_t * shm, semaphores_t * sems, int status);

#endif