#include "worker.h"
#include "worker_queue.h" // Essencial para worker_queue_t e initqueue
#include "thread_pool.h"  // Essencial para thread_pool_t e create_thread_pool
#include "logger.h"
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>       // Essencial para SIGINT
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>      // Essencial para pthread_mutex_lock

#define RED "\033[31m"
#define RESET "\033[0m"

int shutdown_signal = 0;
thread_pool_t * pool;

void signal_worker(int signum){
    (void)signum; // Suppress unused warning
    shutdown_signal = 1;
    if(pool) destroy_thread_pool(pool);
    exit(0);
}

// Função auxiliar para extrair o FD da mensagem de socket
int recv_fd(int socket) {
    struct msghdr msg = {0};
    char dummy;
    struct iovec iov = { .iov_base = &dummy, .iov_len = 1 };
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    char cmsg_buf[CMSG_SPACE(sizeof(int))];
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    // Bloqueante: O worker só chega aqui se o semáforo disser que HÁ dados.
    ssize_t n = recvmsg(socket, &msg, 0);
    if (n <= 0) {
        return -1;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg != NULL && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
        return *(int*)CMSG_DATA(cmsg);
    }
    return -1;
}

void worker_main(shared_memory_t * shm, semaphores_t * sems, int ipc_socket){

    // Inicializar logger localmente
    logger_init(shm->conf.log_file, sems->log_mutex);

    // Configurar sinal para shutdown limpo
    struct sigaction sa;
    sa.sa_handler = signal_worker;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Inicializar fila local
    worker_queue_t lq;
    initqueue(&lq);

    // Criar Pool de Threads
    pool = create_thread_pool(shm->conf.thread_per_worker, &lq, shm, sems);
    printf("Worker %d ready.\n", getpid());

    while (!shutdown_signal)
    {
        // 1. SINCRONIZAÇÃO LÓGICA (Semáforo)
        // Bloqueia até o Master garantir que pôs algo na "mesa"
        if (sem_wait(sems->filled_slots) != 0) break;

        if (shutdown_signal) break;

        // Atualizar índices da fila partilhada (Gestão de pointers circulares)
        sem_wait(sems->queue_mutex);
        shm->queue.front = (shm->queue.front + 1) % MAX_QUEUE_SIZE;
        shm->queue.count--;
        sem_post(sems->queue_mutex);

        // Libertar espaço lógico para o Master (dizemos "já tirei o meu")
        sem_post(sems->empty_slots);

        // 2. LEITURA FÍSICA (Socket)
        // Ler o descritor real do canal partilhado
        int client_fd = recv_fd(ipc_socket);

        if (client_fd < 0) {
            // Se falhar aqui, é grave (dessincronização), mas não mata o worker
            perror(RED "Worker failed to receive FD" RESET);
            continue;
        }

        // 3. PROCESSAMENTO (Thread Pool)
        pthread_mutex_lock(&pool->mutex);
        if (!isFull(&lq)) {
            enqueue(&lq, client_fd);
            pthread_cond_signal(&pool->cond);
        } else {
            // Fila local cheia: o Worker está sobrecarregado.
            close(client_fd);
        }
        pthread_mutex_unlock(&pool->mutex);
    }

    // Cleanup se sair do loop
    destroy_thread_pool(pool);
}