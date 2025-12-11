#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define NUM_THREADS 100
#define NUM_REQUESTS_PER_THREAD 100
#define TOTAL_EXPECTED_REQUESTS (NUM_THREADS * NUM_REQUESTS_PER_THREAD)
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define REQUEST "GET /index.html HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nConnection: close\r\n\r\n"

long successful_requests = 0;
pthread_mutex_t counter_mutex;

void* send_requests(void* UNUSED_ARG) {
    (void)UNUSED_ARG;
    int i;
    for (i = 0; i < NUM_REQUESTS_PER_THREAD; i++) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) continue;

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

        // Se falhar a conexão, ignora silenciosamente e continua
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            close(sockfd);
            continue;
        }

        if (send(sockfd, REQUEST, strlen(REQUEST), 0) < 0) {
            close(sockfd);
            continue;
        }

        char buffer[1024];
        if (recv(sockfd, buffer, sizeof(buffer), 0) > 0) {
            pthread_mutex_lock(&counter_mutex);
            successful_requests++;
            pthread_mutex_unlock(&counter_mutex);
        }

        close(sockfd);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&counter_mutex, NULL);

    printf("--- Starting Concurrency Test (Pura) ---\n");
    printf("Expected total requests: %d\n", TOTAL_EXPECTED_REQUESTS);

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, send_requests, NULL) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            break;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Test finished. Success: %ld/%d\n", successful_requests, TOTAL_EXPECTED_REQUESTS);

    if (successful_requests < TOTAL_EXPECTED_REQUESTS * 0.90) {
        printf("[FAIL] Significant request loss.\n");
        return 1;
    }

    printf("[PASS] Server handled concurrent load successfully.\n");
    
    pthread_mutex_destroy(&counter_mutex);
    return 0;
}