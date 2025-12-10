#include "worker.h"

#include "logger.h"

#define GREY "\033[37m"
#define RESET "\033[0m"

// A map to translate master FDs to local FDs.
// We assume file descriptors won't exceed this value.
#define FD_MAP_SIZE 1024

// Struct to pass arguments to our sync thread
typedef struct {
    int master_socket;
    int* fd_map;
    pthread_mutex_t* map_mutex;
} pull_sync_args_t;

// The recv_fd_from_master function is no longer needed,
// as its logic is now inside pull_recv_sync.

int dequeue_shm(shared_memory_t * shm, semaphores_t * sems){

    sem_wait(sems->filled_slots);
    sem_wait(sems->queue_mutex);

    //dequeue from shm
    int fd = shm->queue.socket_fds[shm->queue.front];
    shm->queue.front = (shm->queue.front + 1) % MAX_QUEUE_SIZE;
    shm->queue.count--;

    sem_post(sems->queue_mutex);
    sem_post(sems->empty_slots);

    return fd;

}

void * pull_recv_sync(void * arg);
void signal_worker();
int shutdown_signal;
thread_pool_t * pool;

void worker_main(shared_memory_t * shm, semaphores_t * sems, int master_socket){

    logger_init("log_acess", sems->log_mutex);

    // This map will store the translation from the master's FD to this worker's FD.
    int fd_map[FD_MAP_SIZE];
    for (int i = 0; i < FD_MAP_SIZE; ++i) {
        fd_map[i] = -1; // Initialize with an invalid value.
    }
    pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

    shutdown_signal=0;
    signal(SIGINT, signal_worker);

    // This thread's job is to receive FDs from the master.
    pull_sync_args_t sync_args = {
        .master_socket = master_socket,
        .fd_map = fd_map,
        .map_mutex = &map_mutex
    };
    pthread_t sync_tid;
    pthread_create(&sync_tid, NULL, pull_recv_sync, &sync_args);
    pthread_detach(sync_tid); // This thread can run independently in the background.

    //Local queue
    worker_queue_t lq;
    initqueue(&lq);

    pool = create_thread_pool(shm->conf.thread_per_worker, &lq, shm, sems);
    

    while (!shutdown_signal)
    {
        // 1. Get the master's original FD number from shared memory.
        int master_fd = dequeue_shm(shm, sems);
        if (master_fd < 0 || master_fd >= FD_MAP_SIZE) continue; // Invalid FD received

        // 2. Look up the real, usable FD in our map.
        // The map is populated by the pull_recv_sync thread.
        int local_fd = -1;
        while (local_fd == -1 && !shutdown_signal) {
            pthread_mutex_lock(&map_mutex);
            local_fd = fd_map[master_fd];
            pthread_mutex_unlock(&map_mutex);
            if (local_fd == -1) {
                // The sync thread might not have received the FD yet. Wait briefly.
                usleep(100);
            }
        }
        if (shutdown_signal) break;

        // 3. Invalidate the map entry so it's not used again.
        pthread_mutex_lock(&map_mutex);
        fd_map[master_fd] = -1;
        pthread_mutex_unlock(&map_mutex);

        // 4. Enqueue the *real* FD for the thread pool to process.
        enqueue(&lq, local_fd);
        printf(GREY"DEBUG: worker.c mapped master_fd %d to local_fd %d and enqueued"RESET"\n", master_fd, local_fd);
        pthread_cond_signal(&pool->cond);   //signal its no longer empty
    }

}

void * pull_recv_sync(void * arg){
    pull_sync_args_t* args = (pull_sync_args_t*)arg;

    while (!shutdown_signal)
    {
        struct msghdr msg = {0};
        int master_fd_val; // This will receive the original FD number from the master.
        struct iovec iov = {.iov_base = &master_fd_val, .iov_len = sizeof(int)};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        char cmsg_buf[CMSG_SPACE(sizeof(int))];
        msg.msg_control = cmsg_buf;
        msg.msg_controllen = sizeof(cmsg_buf);

        if (recvmsg(args->master_socket, &msg, 0) <= 0) {
            break; // Error or connection closed
        }

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg != NULL && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int new_local_fd = *(int*)CMSG_DATA(cmsg);
            if (master_fd_val >= 0 && master_fd_val < FD_MAP_SIZE) {
                // Populate the map: master's FD -> new local FD
                pthread_mutex_lock(args->map_mutex);
                args->fd_map[master_fd_val] = new_local_fd;
                pthread_mutex_unlock(args->map_mutex);
            } else {
                close(new_local_fd); // Invalid master FD, close the handle to prevent leaks
            }
        }
    }
    return NULL;
}

void signal_worker(){
    shutdown_signal=1;

    destroy_thread_pool(pool);

    exit(0);
}
