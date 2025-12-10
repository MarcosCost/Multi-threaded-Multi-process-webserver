#include "master.h"
#include "logger.h"
#include "stats.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h> // Necessário para nanosleep

#define RED "\033[31m"
#define RESET "\033[0m"

// Função para enviar FD sem bloquear
// Retorna 0 em sucesso, -1 em erro crítico, -2 se buffer cheio (EAGAIN)
int send_fd(int socket, int fd_to_send) {
    struct msghdr msg = {0};
    char dummy = '$'; // Payload dummy obrigatório
    struct iovec iov = { .iov_base = &dummy, .iov_len = 1 };
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    char cmsg_buf[CMSG_SPACE(sizeof(int))];
    memset(cmsg_buf, 0, sizeof(cmsg_buf));
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(int));

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *(int*)CMSG_DATA(cmsg) = fd_to_send;

    // MSG_NOSIGNAL: Evita crash se worker morrer (SIGPIPE)
    // MSG_DONTWAIT: Retorna erro imediatamente se buffer estiver cheio (EAGAIN)
    if (sendmsg(socket, &msg, MSG_NOSIGNAL | MSG_DONTWAIT) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -2; // Buffer cheio
        }
        perror("sendmsg error");
        return -1;
    }
    return 0; // Sucesso
}

void master_main(int server_fd, shared_memory_t * shm, semaphores_t * sems, int ipc_socket, config_t * conf){
    (void)conf;
    printf("Master running on port %d (Non-blocking Mode)...\n", conf->port);

    while(1){
        // Aceitar conexão
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            // Se o sistema estiver sem FDs, pausa breve para recuperar
            if (errno == EMFILE || errno == ENFILE) {
                // CORREÇÃO: Usar nanosleep em vez de usleep (POSIX 2008 compliant)
                struct timespec ts = {0, 10000000}; // 10ms em nanosegundos
                nanosleep(&ts, NULL);
            }
            continue;
        }

        // --- 1. Verificar Semáforos (Fila Lógica) ---
        if (sem_trywait(sems->empty_slots) == -1) {
            // Fila cheia -> 503 Service Unavailable
            send_http_response(client_fd, 503, "Service Unavailable", "text/html", "<h1>503 Queue Full</h1>", 21);
            close(client_fd);

            // Stats de rejeição
            add_request_total(shm, sems);
            add_status_code(shm, sems, 503);
            continue;
        }

        // --- 2. Tentar enviar para o Socket Partilhado (Fila Física) ---
        int status = send_fd(ipc_socket, client_fd);

        if (status == 0) {
            // SUCESSO: O FD está no buffer.

            // Tracking na Shared Memory (apenas para debug/stats internos)
            sem_wait(sems->queue_mutex);
            shm->queue.socket_fds[shm->queue.rear] = client_fd;
            shm->queue.rear = (shm->queue.rear + 1) % MAX_QUEUE_SIZE;
            shm->queue.count++;
            sem_post(sems->queue_mutex);

            // SINALIZAR WORKERS: "Há garfos na mesa!"
            sem_post(sems->filled_slots);

            add_connection(shm, sems);
            add_request_total(shm, sems);
        }
        else {
            // FALHA: Buffer cheio (-2) ou erro (-1).
            // Temos de "devolver" o slot do semáforo que ocupámos em cima.
            sem_post(sems->empty_slots);

            // Rejeitar cliente com 503
            send_http_response(client_fd, 503, "Service Unavailable", "text/html", "<h1>503 Socket Full</h1>", 22);
            close(client_fd);

            add_request_total(shm, sems);
            add_status_code(shm, sems, 503);
        }

        // Master já não precisa deste FD (já o enviou ou fechou)
        close(client_fd);
    }
}