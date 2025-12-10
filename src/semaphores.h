/// semaphores - A simple ADT to store the semaphores
///
/// Marcos Costa 125882
/// José Mendes  114429
/// 2025
#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <semaphore.h>

#include <fcntl.h> 

typedef struct 
{
    sem_t* filled_slots;
    sem_t* empty_slots;
    sem_t* queue_mutex;
    sem_t* stats_mutex;
    sem_t* log_mutex;bra
} semaphores_t;

int initialize_semaphores(semaphores_t * sems, int queue_size);

void destroy_semaphores(semaphores_t * sems);

#endif