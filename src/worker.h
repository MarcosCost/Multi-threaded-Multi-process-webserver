#ifndef WORKER_H
#define WORKER_H

#include <sys/un.h> // Adiciona isto se faltar
#include "shared_mem.h"
#include "semaphores.h"

// A assinatura TEM de ser igual ao .c: (..., int ipc_socket)
void worker_main(shared_memory_t * shm, semaphores_t * sems, int ipc_socket);

#endif