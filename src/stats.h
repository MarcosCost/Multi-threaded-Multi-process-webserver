#ifndef STATS_H
#define STATS_H

#include "shared_mem.h"
#include "semaphores.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// Thread que exibe as estatísticas do servidor a cada 30 segundos
void * start_stats(void *arg);

// Funções para atualização segura (atómica) das estatísticas partilhadas
void add_connection(shared_memory_t * shm, semaphores_t * sems);
void remove_connection(shared_memory_t * shm, semaphores_t * sems);
void add_request_total(shared_memory_t * shm, semaphores_t * sems);
void add_bytes_transferred(shared_memory_t * shm, semaphores_t * sems, int bytes);
void add_status_code(shared_memory_t * shm, semaphores_t * sems, int status);

#endif