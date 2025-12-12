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

// Envia um descritor de ficheiro (FD) através de um socket Unix
int send_fd(int socket, int fd_to_send);

// Função principal do processo Master (Loop de aceitação)
void master_main(int server_fd, shared_memory_t * shm, semaphores_t * sems, int ipc_socket, config_t * conf);

#endif