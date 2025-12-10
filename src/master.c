#include "master.h"
#include "logger.h"

#include "stats.h"

#define GREY "\033[37m"
#define RESET "\033[0m"

void send_fd_to_worker(int worker_socket, int fd_to_send) {
    struct msghdr msg = {0};
    struct iovec iov = { .iov_base = &fd_to_send, .iov_len = sizeof(int) }; // This is the main data payload
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    // Ancillary data for the fd
    char cmsg_buf[CMSG_SPACE(sizeof(int))];
    memset(cmsg_buf, 0, sizeof(cmsg_buf));
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(int));

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *(int*)CMSG_DATA(cmsg) = fd_to_send;

    sendmsg(worker_socket, &msg, 0);
}

void master_main(int server_fd, shared_memory_t * shm, semaphores_t * sems, int (*worker_sockets)[2] , config_t * conf){

    while(1){

        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept failed in master");
            continue;
        }

        add_connection(shm, sems);
        add_request_total(shm, sems);
        

        if (sem_trywait(sems->empty_slots) == -1) {
            // This is the non-blocking way to check if the queue is full.

            //Read HTML file
            FILE *file = fopen("www/503.html", "rb");
            if (file == NULL) {
                send_http_response(client_fd, 503, "Service Unavailable", "text/html","503 Service Unavailable", 24);
            } else {
                if (fseek(file, 0, SEEK_END) == 0) {
                    long file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    char *html_body = malloc(file_size);
                    if (html_body) {
                        fread(html_body, 1, file_size, file);
                        send_http_response(client_fd, 503, "Service Unavailable", "text/html", html_body, file_size);
                        add_bytes_transferred(shm, sems, file_size);
                        free(html_body);
                    } else {
                        send_http_response(client_fd, 503, "Service Unavailable", "text/html","503 Service Unavailable", 24);

                        add_bytes_transferred(shm, sems, 24);
                    }
                }
                fclose(file);
            }

            logger_log_request("127.0.0.1", "GET", "/503.html", 503, 24);
            add_status_code(shm, sems, 503);
            close(client_fd);
            remove_connection(shm, sems);
            continue;
        }
        
        // Send fd to all workers
        for (int i = 0; i < conf->num_workers; i++) {
            send_fd_to_worker(worker_sockets[i][1], client_fd);
        }

        sem_wait(sems->queue_mutex);
        shm->queue.socket_fds[shm->queue.rear] = client_fd;
        shm->queue.rear = (shm->queue.rear + 1) % MAX_QUEUE_SIZE;
        shm->queue.count++;
        sem_post(sems->queue_mutex);

        sem_post(sems->filled_slots);
        printf(GREY"DEBUG: master.c enqueued %d"RESET"\n", client_fd);

        close(client_fd);  // master doesn't need it anymore
    }

}