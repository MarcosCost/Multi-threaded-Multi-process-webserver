#ifndef WORKER_H
#define WORKER_H

#include <sys/un.h>
#include "shared_mem.h"
#include "semaphores.h"

// Função principal do processo Worker (Loop de consumo)
void worker_main(shared_memory_t * shm, semaphores_t * sems, int ipc_socket);

#endif