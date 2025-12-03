#include "worker_queue.h"

// Function to initialize the queue
void initqueue(worker_queue_t* q)
{
    q->front = 0;
    q->back = 0;
    q->size = 0;
}

// Function to check if the queue is empty
int isEmpty(worker_queue_t* q) { return (q->size == 0); }

// Function to check if the queue is full
int isFull(worker_queue_t* q) { return (q->size == MAX_LOCAL_SIZE); }

// Function to add an element to the queue (Enqueue
// operation)
void enqueue(worker_queue_t* q, int value)
{
    if (isFull(q)) {
        printf("Queue is full\n");
        return;
    }
    q->arr[q->back] = value;
    q->back = (q->back + 1) % MAX_LOCAL_SIZE;  // Circular increment
    q->size++;
}   

// Function to remove an element from the queue (Dequeue
// operation)
int dequeue(worker_queue_t* q)
{
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return -1;
    }
    int value = q->arr[q->front];
    q->front = (q->front + 1) % MAX_LOCAL_SIZE;
    q->size--;
    return value;
}