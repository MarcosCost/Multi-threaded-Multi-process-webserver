// thread_pool.c
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
        pthread_mutex_lock(&pool->mutex);

        while (!pool->shutdown  &&  isEmpty(pool->worker_queue)) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        if (pool->shutdown && isEmpty(pool->worker_queue)) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }

        // Dequeue work item and process
        int fd = dequeue(pool->worker_queue);

        pthread_mutex_unlock(&pool->mutex);

        char buff[4096];
        ssize_t bytes_recv = recv(fd, buff, sizeof(buff) - 1, 0);

        http_request_t request;

        int status = 200; // Default status
        char status_message[20];
        char mime_type[50];
        char * body = NULL; // --- CORREÇÃO: Inicializar a NULL ---
        size_t body_size = 0;

        if (bytes_recv > 0) {
            buff[bytes_recv] = '\0'; // Garantir null-termination

            if (parse_http_request(buff, &request) == 0) {

                // --- CORREÇÃO: Buffer maior para o caminho ---
                char path[512];
                // Usar a root configurada (pool->shm->conf.document_root)
                build_full_path(pool->shm->conf.document_root, request.path, path, sizeof(path));

                // Lógica simples para Index: Se o path acaba em /, adiciona index.html
                // Ou se o request.path for apenas "/", assumimos index.html
                if (strcmp(request.path, "/") == 0) {
                    strncat(path, "index.html", sizeof(path) - strlen(path) - 1);
                }

                printf("DEBUG: Request: %s -> Path: %s\n", request.path, path);

                if (path_exists(path)) {
                    status = 200;
                    strcpy(status_message, "OK");

                    // Determinar MIME type
                    strcpy(mime_type, get_mimetype(path));

                    // Abrir e ler ficheiro
                    FILE *file = fopen(path, "rb");
                    if (file == NULL) {
                        perror("Couldnt open file");
                        status = 500; // Fallback se falhar abertura
                    } else {
                        fseek(file, 0, SEEK_END);
                        long file_size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        body = malloc(file_size);
                        if (body) {
                            fread(body, 1, file_size, file);
                            body_size = file_size;
                        }
                        fclose(file);
                    }

                } else {
                    // --- STATUS 404 NOT FOUND ---
                    status = 404;
                    strcpy(status_message, "NOT FOUND");
                    strcpy(mime_type, "text/html");

                    // Construir caminho para 404.html usando a root configurada
                    char path_404[512];
                    snprintf(path_404, sizeof(path_404), "%s404.html", pool->shm->conf.document_root);

                    FILE *file = fopen(path_404, "rb");
                    if (file == NULL) {
                        // Fallback simples se não houver 404.html personalizado
                        char *fallback_404 = "<h1>404 Not Found</h1>";
                        body = malloc(strlen(fallback_404) + 1);
                        if(body) {
                            strcpy(body, fallback_404);
                            body_size = strlen(body);
                        }
                    } else {
                        fseek(file, 0, SEEK_END);
                        long file_size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        body = malloc(file_size);
                        if (body) {
                            fread(body, 1, file_size, file);
                            body_size = file_size;
                        }
                        fclose(file);
                    }
                }
            } else {
                // Erro no parse
                status = 400;
                strcpy(status_message, "Bad Request");
            }

        } else if(bytes_recv == 0) {
            /*Client closed connection before processing*/
        } else {
            perror("recv failed in worker thread");
        }


        // Enviar resposta
        // Se body for NULL (ex: HEAD request ou erro sem corpo), enviamos tamanho 0
        if (body != NULL) {
            if (strcmp(request.method, "HEAD") != 0) {
                send_http_response(fd, status, status_message, mime_type, body, body_size);
            } else {
                // HEAD request: envia headers mas não corpo
                send_http_response(fd, status, status_message, mime_type, NULL, 0);
            }
            add_bytes_transferred(pool->shm, pool->sems, body_size);

            
        } else {
            // Resposta vazia (pode acontecer em erros graves ou HEAD)
            send_http_response(fd, status, status_message, (status == 200 ? mime_type : "text/html"), NULL, 0);
            add_bytes_transferred(pool->shm, pool->sems, 0);
        }

        logger_log_request("127.0.0.1", request.method, request.path, status, body_size);
        add_status_code(pool->shm, pool->sems, status);

        add_status_code(pool->shm, pool->sems, status);

        if (body != NULL) {
            free(body);
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
        // printf("Thread %d joined\n", i); // Opcional: remover print de debug em produção
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool);
}