/// Config - A simple ADT to Parse and store server.config values
///
/// Marcos Costa 125882
/// José Mendes  114429
/// 2025
#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <string.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <unistd.h>
#include <config.h>

#define SHM_NAME "/webserver_shm"
#define MAX_QUEUE_SIZE 100

typedef struct
{
    int socket_fds[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
} connection_queue_t;
typedef struct 
{
    long total_requests;
    long bytes_transferred;
    long status_200;    //OK
    long status_404;    //NOT FOUND
    long status_500;    //INTERNAL SERVER ERRORS
    int active_connections;
} server_stats_t;


typedef struct
{   
    config_t conf;
    connection_queue_t queue;
    server_stats_t stats;
} shared_memory_t;

//Creates a shared memory segment and returns it's pointer, returns null if not initialized properly
shared_memory_t * create_shared_memory();

void destroy_shared_memory(shared_memory_t * data);

#endif