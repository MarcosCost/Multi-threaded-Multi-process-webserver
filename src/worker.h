#ifndef WORKER_H
#define WORKER_H

#include <sys/un.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include "worker_queue.h"
#include "thread_pool.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "http.h"
#include "worker_queue.h"
#include "thread_pool.h"

int recv_fd_from_master(int master_socket);

void worker_main(shared_memory_t * shm, semaphores_t * sems, int master_socket);

#endif