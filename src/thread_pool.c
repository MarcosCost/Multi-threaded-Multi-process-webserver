// thread_pool.c
#include "thread_pool.h"
#include "http.h"
#include "stats.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>


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
        
        char buff[4000];
        ssize_t bytes_recv =  recv(fd, buff, sizeof(buff) -1, 0);

        http_request_t request;

        int status;
        char status_message[20];
        char mime_type[30];            
        char * body;  //what we are actually sending ex: html page, text, etc...
        size_t body_size;

        if (bytes_recv > 0){               

            parse_http_request(buff, &request);
                
            //build full path
            char path[30];
            build_full_path("www", request.path, path, 30);
            printf("request.path = %s\n built path = %s\n", request.path, path);
            
            if (path_exists(path)){
                status = 200;
                strcpy(status_message, "OK");
                
                if (strcmp(path, "www/") == 0)
                {
                    strcpy(mime_type, "html");
                    //open file and load it to the body
                    FILE *file = fopen("www/index.html", "rb");
                    if (file == NULL)
                    {   
                        perror("Couldnt open file");
                    } else {
                        //find size
                        fseek(file, 0, SEEK_END);
                        long file_size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        body = malloc(file_size);

                        fread(body, 1, file_size, file);
                        body_size = file_size;
                    }
                    fclose(file);
                } else {
                    
                    strcpy(mime_type, get_mimetype(path));

                    //open file and load it to the body

                    FILE *file = fopen(path, "rb");
                    if (file == NULL)
                    {   
                        perror("Couldnt open file");
                    } else {
                        //find size
                        fseek(file, 0, SEEK_END);
                        long file_size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        body = malloc(file_size);

                        fread(body, 1, file_size, file);
                        body_size = file_size;
                    }
                    fclose(file);

                }

            } else {
                
                //STATUS 404 NOT FOUND
                status = 404;
                strcpy(status_message, "NOT FOUND");
                strcpy(mime_type, "html");
                //open file and load it to the body
                FILE *file = fopen("www/404.html", "rb");
                if (file == NULL)
                {   
                    perror("Couldnt open file");
                } else {
                    //find size
                    fseek(file, 0, SEEK_END);
                    long file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    body = malloc(file_size);

                    fread(body, 1, file_size, file);
                    body_size = file_size;
                }
                fclose(file);
            }

        } else if(bytes_recv == 0) {
            /*Client closed connection before processing*/
        } else{
            perror("recv failed in worker thread");
        }


        //send the response
        if (strcmp(request.method,"GET") == 0)
        {
            send_http_response(fd, status, status_message, mime_type, body, body_size);
            add_bytes_transferred(pool->shm, pool->sems, body_size);
        } else {
            send_http_response(fd, status, status_message, mime_type, NULL, 0);
            add_bytes_transferred(pool->shm, pool->sems, 0);
        }
        add_status_code(pool->shm, pool->sems, status);

        free(body);


        close(fd);
        remove_connection(pool->shm, pool->sems);
        

    }
    
    return NULL;
}

thread_pool_t* create_thread_pool(int num_threads, worker_queue_t* queue, shared_memory_t * shm, semaphores_t * sems) {
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));    
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
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex);
    
    pthread_cond_broadcast(&pool->cond);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
        printf("Thread %d joined\n", i);
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);   
    free(pool);
}
