#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <semaphore.h>
#include <fcntl.h>

typedef struct {
    sem_t* filled_slots;
    sem_t* empty_slots;
    sem_t* queue_mutex;
    sem_t* stats_mutex;
    sem_t* log_mutex;
} semaphores_t;

// Inicializa os semáforos nomeados POSIX
// Retorna 0 em sucesso, -1 em erro
int initialize_semaphores(semaphores_t * sems, int queue_size);

// Fecha (sem_close) e remove (sem_unlink) os semáforos do sistema
void destroy_semaphores(semaphores_t * sems);

#endif