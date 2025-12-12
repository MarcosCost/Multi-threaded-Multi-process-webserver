#include "stats.h"

void * start_stats(void *arg) {
    shared_memory_t *stats = (shared_memory_t *)arg;

    while (1)
    {
        // Aguarda 30 segundos antes de atualizar o ecrã
        sleep(30);

        printf("----------------------------------\n");
        printf("Active Connections: %d\n",  stats->stats.active_connections);
        printf("Total Requests:     %ld\n", stats->stats.total_requests);
        printf("Bytes Transferred:  %ld\n", stats->stats.bytes_transferred);
        printf("----------------------------------\n");
        printf("Status Codes:\n");
        printf("  [200] OK:         %ld\n", stats->stats.status_200);
        printf("  [404] Not Found:  %ld\n", stats->stats.status_404);
        printf("  [500] Error:      %ld\n", stats->stats.status_500);
        printf("----------------------------------\n");
    }

    return NULL;
}

void add_connection(shared_memory_t * shm, semaphores_t * sems) {
    sem_wait(sems->stats_mutex);
    shm->stats.active_connections++;
    sem_post(sems->stats_mutex);
}

void remove_connection(shared_memory_t * shm, semaphores_t * sems) {
    sem_wait(sems->stats_mutex);
    shm->stats.active_connections--;
    sem_post(sems->stats_mutex);
}

void add_request_total(shared_memory_t * shm, semaphores_t * sems) {
    sem_wait(sems->stats_mutex);
    shm->stats.total_requests++;
    sem_post(sems->stats_mutex);
}

void add_bytes_transferred(shared_memory_t * shm, semaphores_t * sems, int bytes) {
    sem_wait(sems->stats_mutex);
    shm->stats.bytes_transferred += bytes;
    sem_post(sems->stats_mutex);
}

void add_status_code(shared_memory_t * shm, semaphores_t * sems, int status) {
    sem_wait(sems->stats_mutex);
    switch (status)
    {
        case 200:
            shm->stats.status_200++;
            break;
        case 404:
            shm->stats.status_404++;
            break;
        case 500:
            shm->stats.status_500++;
            break;
        default:
            break;
    }
    sem_post(sems->stats_mutex);
}