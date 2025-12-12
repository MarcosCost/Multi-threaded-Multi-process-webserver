#include "worker_queue.h"

void initqueue(worker_queue_t* q) {
    q->front = 0;
    q->back = 0;
    q->size = 0;
}

int isEmpty(worker_queue_t* q) {
    return (q->size == 0);
}

int isFull(worker_queue_t* q) {
    return (q->size == MAX_LOCAL_SIZE);
}

// Adicionar elemento (Lógica Circular)
void enqueue(worker_queue_t* q, int value) {
    if (isFull(q)) {
        return;
    }
    q->arr[q->back] = value;
    q->back = (q->back + 1) % MAX_LOCAL_SIZE;
    q->size++;
}

// Remover elemento (Lógica Circular)
int dequeue(worker_queue_t* q) {
    if (isEmpty(q)) {
        return -1;
    }
    int value = q->arr[q->front];
    q->front = (q->front + 1) % MAX_LOCAL_SIZE;
    q->size--;
    return value;
}