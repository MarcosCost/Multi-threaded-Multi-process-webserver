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

// Função auxiliar para enviar FD (usada internamente, mas pode estar aqui)
int send_fd(int socket, int fd_to_send);

// CORREÇÃO: Recebe apenas um 'int ipc_socket' em vez do array 'worker_sockets'
void master_main(int server_fd, shared_memory_t * shm, semaphores_t * sems, int ipc_socket, config_t * conf);

#endif