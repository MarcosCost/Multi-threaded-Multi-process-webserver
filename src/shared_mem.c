#include "shared_mem.h"

shared_memory_t * create_shared_memory(){
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1) return NULL;

    // Definir o tamanho do segmento de memória partilhada
    if (ftruncate(shm_fd, sizeof(shared_memory_t)) == -1 ){
        close(shm_fd);
        return NULL;
    }

    shared_memory_t * data = mmap(NULL, sizeof(shared_memory_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(data == MAP_FAILED) return NULL;

    memset(data, 0, sizeof(shared_memory_t));

    // Inicializar array de sockets com -1 (inválido)
    for(int i = 0; i < MAX_QUEUE_SIZE; i++){
        data->queue.socket_fds[i] = -1;
    }

    return data;
}

void destroy_shared_memory(shared_memory_t* data){
    if (data != NULL) {
        // Limpar dados antes de desmontar (segurança)
        memset(data, 0, sizeof(shared_memory_t));
        munmap(data, sizeof(shared_memory_t));
    }
    shm_unlink(SHM_NAME);
}