#include "config.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "master.h"
#include "worker.h"
#include "stats.h"

#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#define RED "\033[31m"
#define RESET "\033[0m"
#define GREEN "\033[32m"
#define PURPLE "\033[35m"

config_t conf;
shared_memory_t * shm;
semaphores_t sems;
pid_t * pids;

pthread_t stats_thread;

void signal_handler(int);
int create_server_socket(int port);

int main(void){

    printf(PURPLE"\n=== WEB SERVER STARTING (Shared Socket Architecture) ==="RESET"\n\n");

    ///////////////////////////////////////////////
    // Common Resources and signal handler Setup //
    ///////////////////////////////////////////////

    if (initConfig(&conf) == -1)
    {
        printf( RED "Error loading configurations"RESET"\n");
        return -1;
    }
    printf(GREEN"Configurations loaded sucessfully"RESET"\n");

    shm = create_shared_memory();
    if (shm == NULL)
    {
        printf(RED"Error setting up shared memory"RESET"\n");
        return -1;
    }
    printf(GREEN"Shared Memory setup sucessfully"RESET"\n");

    if (initialize_semaphores(&sems, MAX_QUEUE_SIZE) == -1)
    {
        printf(RED"Error initializing semaphores"RESET"\n");
        return -1;
    }
    printf(GREEN"Semaphores initialized sucessfully"RESET"\n");

    // Copy config to shared memory so workers can access document_root
    shm->conf = conf;

    //Setup signal handler
    signal(SIGINT, signal_handler);

    int server_socket = create_server_socket(conf.port);
    if (server_socket < 0)
    {
        printf(RED"Error creating server socket"RESET"\n");
        return -1;
    }

    //stats display thread initialization
    shm->stats.active_connections = 0;
    shm->stats.total_requests = 0;
    shm->stats.bytes_transferred = 0;
    shm->stats.status_200 = 0;
    shm->stats.status_404 = 0;
    shm->stats.status_500 = 0;

    pthread_create(&stats_thread, NULL, start_stats, shm);

    // --- CORREÇÃO DEADLOCK: Socket Pair Único e Partilhado ---
    // Em vez de N canais, usamos um canal partilhado.
    // [0] Leitura (Workers), [1] Escrita (Master)
    int ipc_socket[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipc_socket) == -1) {
        perror("socketpair");
        return -1;
    }

    // Aumentar buffer para 512KB para aguentar picos de carga (opcional mas recomendado)
    int buff_size = 512 * 1024;
    setsockopt(ipc_socket[1], SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size));
    setsockopt(ipc_socket[0], SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size));

    //////////////////
    // Fork Workers //
    //////////////////

    pids = malloc(conf.num_workers * sizeof(pid_t));

    for (int i = 0; i < conf.num_workers; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            // --- WORKER PROCESS ---
            close(server_socket);

            // O Worker apenas LÊ do socket partilhado
            close(ipc_socket[1]);

            printf("Initializing worker %d\n", i);

            // Passamos o socket de leitura partilhado
            worker_main(shm, &sems, ipc_socket[0]);

            free(pids);
            exit(0);
        }
        else if (pid > 0)
        {
            // --- MASTER PROCESS ---
            pids[i] = pid;
        }
        else
        {
            perror("Fork failed");
            exit(1);
        }
    }

    // --- MASTER MAIN LOOP ---
    // O Master apenas ESCREVE no socket partilhado
    close(ipc_socket[0]);
    master_main(server_socket, shm, &sems, ipc_socket[1], &conf);

    return 0;
}

void signal_handler(int signum){

    printf("\r  ");
    printf(PURPLE"\n=== SHUTTING DOWN SERVER on signal"RED" %d "PURPLE"==="RESET"\n", signum);

    printf("  Terminating worker processes...\n");
    if (pids != NULL) {
        for (int i = 0; i < conf.num_workers; i++) {
            if (pids[i] > 0) {
                kill(pids[i], SIGTERM);
            }
        }
        // Esperar efetivamente que morram para evitar zombies
        while(wait(NULL) > 0);
        free(pids);
    }
    printf("  Workers terminated.\n");

    printf("  Destroying shared memory:\n");
    destroy_shared_memory(shm);
    printf("  Done\n");

    printf("  Destroying Semaphores:\n");
    destroy_semaphores(&sems);
    printf("  Done\n");

    pthread_cancel(stats_thread);
    pthread_join(stats_thread, NULL);

    exit(0);
}

int create_server_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 1024) < 0) { // Aumentado backlog
        perror("Listen failed");
        close(sockfd);
        return -1;
    }
    return sockfd;
}