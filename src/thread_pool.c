#include "thread_pool.h"
#include "http.h"
#include "stats.h"
#include "logger.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void* worker_thread(void * arg) {
    thread_pool_t* pool = (thread_pool_t*)arg;

    while (1) {
        // 1. Sincronização da Fila (Produtor-Consumidor Local)
        pthread_mutex_lock(&pool->mutex);

        while (!pool->shutdown  &&  isEmpty(pool->worker_queue)) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        if (pool->shutdown && isEmpty(pool->worker_queue)) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }

        int fd = dequeue(pool->worker_queue);
        pthread_mutex_unlock(&pool->mutex);

        // 2. Receção do Pedido
        char buff[4096];
        ssize_t bytes_recv = recv(fd, buff, sizeof(buff) - 1, 0);

        if (bytes_recv <= 0) {
            close(fd);
            continue;
        }

        buff[bytes_recv] = '\0';

        // 3. Processamento HTTP
        http_request_t request;
        int status = 200;
        char status_message[20];
        char mime_type[50];
        char * body = NULL;
        size_t body_size = 0;

        if (parse_http_request(buff, &request) == 0) {

            char path[512];
            build_full_path(pool->shm->conf.document_root, request.path, path, sizeof(path));

            // Lógica para Index
            if (strcmp(request.path, "/") == 0) {
                strncat(path, "index.html", sizeof(path) - strlen(path) - 1);
            }

            // 4. Integração Cache LRU
            // Tenta obter da cache primeiro (Reader Lock)
            if (cache_get(pool->cache, path, (void**)&body, &body_size) == 0) {
                // CACHE HIT
                status = 200;
                strcpy(status_message, "OK");
                strcpy(mime_type, get_mimetype(path));
            }
            else {
                // CACHE MISS - Ler do disco
                if (path_exists(path)) {
                    status = 200;
                    strcpy(status_message, "OK");
                    strcpy(mime_type, get_mimetype(path));

                    FILE *file = fopen(path, "rb");
                    if (file == NULL) {
                        status = 500;
                        strcpy(status_message, "Internal Error");
                    } else {
                        fseek(file, 0, SEEK_END);
                        long file_size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        body = malloc(file_size);
                        if (body) {
                            fread(body, 1, file_size, file);
                            body_size = file_size;

                            // Adicionar à Cache (Writer Lock gerido internamente)
                            cache_put(pool->cache, path, body, body_size);
                        } else {
                            status = 500;
                        }
                        fclose(file);
                    }
                } else {
                    // 404 NOT FOUND
                    status = 404;
                    strcpy(status_message, "NOT FOUND");
                    strcpy(mime_type, "text/html");

                    // Tentar servir 404.html customizado
                    char path_404[512];
                    snprintf(path_404, sizeof(path_404), "%s404.html", pool->shm->conf.document_root);

                    FILE *file = fopen(path_404, "rb");
                    if (file) {
                        fseek(file, 0, SEEK_END);
                        long fsize = ftell(file);
                        fseek(file, 0, SEEK_SET);
                        body = malloc(fsize);
                        if(body) {
                            fread(body, 1, fsize, file);
                            body_size = fsize;
                        }
                        fclose(file);
                    } else {
                        const char *fallback = "<h1>404 Not Found</h1>";
                        body = strdup(fallback);
                        body_size = strlen(fallback);
                    }
                }
            }
        } else {
            status = 400;
            strcpy(status_message, "Bad Request");
        }

        // 5. Enviar Resposta
        if (body != NULL || status == 200 || status == 404 || status == 503) {
            if (strcmp(request.method, "HEAD") != 0) {
                send_http_response(fd, status, status_message, mime_type, body, body_size);
            } else {
                send_http_response(fd, status, status_message, mime_type, NULL, body_size);
            }
            add_bytes_transferred(pool->shm, pool->sems, body_size);
        }

        // 6. Logs e Limpeza
        logger_log_request("127.0.0.1", request.method, request.path, status, body_size);
        add_status_code(pool->shm, pool->sems, status);

        if (body != NULL) {
            free(body); // Liberta a cópia da cache ou o buffer de leitura
        }

        close(fd);
        remove_connection(pool->shm, pool->sems);
    }

    return NULL;
}

thread_pool_t* create_thread_pool(int num_threads, worker_queue_t* queue, shared_memory_t * shm, semaphores_t * sems) {
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    if (!pool) return NULL;

    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    pool->num_threads = num_threads;
    pool->shutdown = 0;
    pool->worker_queue = queue;
    pool->shm = shm;
    pool->sems = sems;

    // Inicialização da Cache (MB -> Bytes)
    size_t cache_capacity_bytes = (size_t)shm->conf.cache_size * 1024 * 1024;
    pool->cache = cache_init(cache_capacity_bytes);

    if (!pool->cache) {
        printf("AVISO: Falha ao inicializar cache. O servidor correrá sem cache.\n");
    }

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }

    return pool;
}

void destroy_thread_pool(thread_pool_t* pool){
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex);

    pthread_cond_broadcast(&pool->cond);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    if (pool->cache) {
        cache_destroy(pool->cache);
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool);
}