#ifndef LQ_H
#define LQ_H

#define MAX_LOCAL_SIZE 100

#include <stdio.h>

typedef struct {
    int arr[MAX_LOCAL_SIZE];
    int back;
    int front;
    int size;
} worker_queue_t;

// Inicializa a fila local
void initqueue(worker_queue_t * q);

// Adiciona um elemento à fila
void enqueue(worker_queue_t* q, int i);

// Remove e retorna um elemento da fila
int dequeue(worker_queue_t* q);

// Verificações de estado
int isFull(worker_queue_t* q);
int isEmpty(worker_queue_t* q);

#endif