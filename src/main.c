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

    printf(PURPLE"\n=== SERVIDOR WEB A INICIAR (Arquitetura Shared Socket) ==="RESET"\n\n");

    // Configuração de Recursos Comuns e Signal Handler
    if (initConfig(&conf) == -1)
    {
        printf(RED "Erro ao carregar configurações"RESET"\n");
        return -1;
    }
    printf(GREEN"Configurações carregadas com sucesso"RESET"\n");

    shm = create_shared_memory();
    if (shm == NULL)
    {
        printf(RED"Erro ao configurar memória partilhada"RESET"\n");
        return -1;
    }
    printf(GREEN"Memória Partilhada configurada com sucesso"RESET"\n");

    if (initialize_semaphores(&sems, MAX_QUEUE_SIZE) == -1)
    {
        printf(RED"Erro ao inicializar semáforos"RESET"\n");
        return -1;
    }
    printf(GREEN"Semáforos inicializados com sucesso"RESET"\n");

    // Copiar config para memória partilhada para acesso dos workers
    shm->conf = conf;

    signal(SIGINT, signal_handler);

    int server_socket = create_server_socket(conf.port);
    if (server_socket < 0)
    {
        printf(RED"Erro ao criar socket do servidor"RESET"\n");
        return -1;
    }

    // Inicialização de estatísticas
    shm->stats.active_connections = 0;
    shm->stats.total_requests = 0;
    shm->stats.bytes_transferred = 0;
    shm->stats.status_200 = 0;
    shm->stats.status_404 = 0;
    shm->stats.status_500 = 0;

    pthread_create(&stats_thread, NULL, start_stats, shm);

    // Socket Pair Único e Partilhado (IPC)
    // [0] Leitura (Workers), [1] Escrita (Master)
    int ipc_socket[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipc_socket) == -1) {
        perror("socketpair");
        return -1;
    }

    // Aumentar buffer para 512KB para aguentar picos
    int buff_size = 512 * 1024;
    setsockopt(ipc_socket[1], SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size));
    setsockopt(ipc_socket[0], SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size));

    // Fork dos Workers
    pids = malloc(conf.num_workers * sizeof(pid_t));

    for (int i = 0; i < conf.num_workers; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            // --- PROCESSO WORKER ---
            close(server_socket);
            close(ipc_socket[1]); // Fecha escrita

            printf("A inicializar worker %d\n", i);

            // Passa o socket de leitura partilhado
            worker_main(shm, &sems, ipc_socket[0]);

            free(pids);
            exit(0);
        }
        else if (pid > 0)
        {
            // --- PROCESSO MASTER ---
            pids[i] = pid;
        }
        else
        {
            perror("Fork falhou");
            exit(1);
        }
    }

    // --- LOOP PRINCIPAL DO MASTER ---
    close(ipc_socket[0]); // Fecha leitura
    master_main(server_socket, shm, &sems, ipc_socket[1], &conf);

    return 0;
}

void signal_handler(int signum){

    printf("\r  ");
    printf(PURPLE"\n=== A ENCERRAR O SERVIDOR sinal"RED" %d "PURPLE"==="RESET"\n", signum);

    printf("  A terminar processos worker...\n");
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
    printf("  Workers terminados.\n");

    printf("  A destruir memória partilhada:\n");
    destroy_shared_memory(shm);
    printf("  Feito\n");

    printf("  A destruir Semáforos:\n");
    destroy_semaphores(&sems);
    printf("  Feito\n");

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
        perror("Bind falhou");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 1024) < 0) {
        perror("Listen falhou");
        close(sockfd);
        return -1;
    }
    return sockfd;
}