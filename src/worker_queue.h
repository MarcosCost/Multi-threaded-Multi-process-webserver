/// worker_queue - A simple ADT  for local worker queues
///
/// Marcos Costa 125882
/// José Mendes  114429
/// 2025

#ifndef LQ_H
#define LQ_H

#define MAX_LOCAL_SIZE 200

#include <stdio.h>

typedef struct
{   
    int arr[MAX_LOCAL_SIZE];
    int back;
    int front;
    int size;  // Added to track queue size
} worker_queue_t ;

void initqueue(worker_queue_t * q);

void enqueue(worker_queue_t* q, int i);

int dequeue(worker_queue_t* q);

int isFull(worker_queue_t* q);
int isEmpty(worker_queue_t* q);



#endif