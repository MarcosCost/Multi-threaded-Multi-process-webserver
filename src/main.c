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

#define RED "\033[31m"
#define RESET "\033[0m"
#define GREEN "\033[32m"
#define PURPLE "\033[35m"

config_t conf;
shared_memory_t * shm;
semaphores_t sems;
pid_t * pids;

server_stats_t stats;
pthread_t stats_thread;

void signal_handler(int);
int create_server_socket(int port);

int main(void){

    printf(PURPLE"\n=== WEB SERVER STARTING ==="RESET"\n\n");

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

    shm->conf = conf;

    //Setup signal handler
    signal(SIGINT, signal_handler);

    int server_socket = create_server_socket(conf.port);
    if (server_socket<0)
    {
        printf(RED"Error creating server socket"RESET"\n");
    }
    

    //stat thread
    stats.active_connections = 0;
    stats.total_requests = 0;
    stats.bytes_transferred = 0;
    stats.start_time = time(NULL);
    for (int i = 0; i < 600; i++) {
        stats.status_counts[i] = 0;
    }

    pthread_create(&stats_thread, NULL, start_stats, &stats);




    int worker_sockets[conf.num_workers][2];  // [read_end, write_end]
    for (int i = 0; i < conf.num_workers; i++) {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, worker_sockets[i]) == -1) {
            perror("socketpair");
            return -1;
        }
    }

    //////////////////
    // Fork Workers //
    //////////////////

    pids = malloc(conf.num_workers*sizeof(pid_t));

    for (int i = 0; i < conf.num_workers; i++)
    {
        
        pid_t pid = fork();
        
        if (pid == 0)
        {
            //Workers
            close(server_socket);

            for (int j = 0; j < conf.num_workers; j++) {
                if (j != i) {
                    close(worker_sockets[j][0]);
                    close(worker_sockets[j][1]);
                }
            }
            close(worker_sockets[i][1]); // Worker closes its own write end
            printf("Initializing worker %d\n", i);    
            worker_main(shm, &sems, worker_sockets[i][0]);
            exit(0);
        }else if (pid > 0)
        {
            pids[i] = pid;
            close(worker_sockets[i][0]);  // parent doesn't need read end
        }

    }
    
    
    master_main(server_socket, shm, &sems, worker_sockets, &conf);

}

void signal_handler(int signum){

    printf("\r  ");
    printf(PURPLE"\n=== SHUTING DOWN SERVER on signal"RED" %d "PURPLE"==="RESET"\n", signum);
        
    printf("  Destrying shared memory:\n");
    destroy_shared_memory(shm);
    printf("  Done\n");
    
    printf("  Destroying Semaphores:\n");
    destroy_semaphores(&sems);
    printf("  Done\n");

    free(pids);

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
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 128) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}