
#ifndef MASTER_H
#define MASTER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>

#include "shared_mem.h"
#include "semaphores.h"
#include "http.h"
#include "config.h"

void send_fd_to_worker(int worker_socket, int fd_to_send);

void master_main(int server_fd, shared_memory_t * shm, semaphores_t * sems, int (*worker_sockets)[2] , config_t * conf);

#endif