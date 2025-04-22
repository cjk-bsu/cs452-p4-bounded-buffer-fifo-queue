#include "lab.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Implementation of the queue struct
 */
struct queue {
    void **buffer;          // Array to store queue elements
    int capacity;           // Maximum capacity of the queue
    int size;               // Current size of the queue
    int front;              // Index of the front element
    int rear;               // Index of the rear element
    bool is_shutdown_flag;  // Flag to indicate if queue is shutdown
    
    // Synchronization primitives
    pthread_mutex_t lock;       // Mutex for critical sections
    pthread_cond_t not_empty;   // Condition variable for queue not empty
    pthread_cond_t not_full;    // Condition variable for queue not full
};

queue_t queue_init(int capacity) {
    if (capacity <= 0) {
        return NULL;  // Invalid capacity
    }
    
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (!q) {
        return NULL;  // Memory allocation failed
    }
    
    q->buffer = (void **)malloc(capacity * sizeof(void *));
    if (!q->buffer) {
        free(q);
        return NULL;  // Memory allocation failed
    }
    
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = -1;
    q->is_shutdown_flag = false;
    
    // Initialize synchronization primitives
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    
    return q;
}

void queue_destroy(queue_t q) {
    if (!q) {
        return;
    }
    
    // Set shutdown flag and signal all waiting threads
    pthread_mutex_lock(&q->lock);
    q->is_shutdown_flag = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    
    // Destroy synchronization primitives
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    
    // Free allocated memory
    free(q->buffer);
    free(q);
}

void enqueue(queue_t q, void *data) {
    if (!q) {
        return;
    }
    
    pthread_mutex_lock(&q->lock);
    
    // Wait until there's space in the queue or the queue is shutdown
    while (q->size == q->capacity && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    
    // If the queue is shutdown, don't enqueue
    if (q->is_shutdown_flag) {
        pthread_mutex_unlock(&q->lock);
        return;
    }
    
    // Add the element to the queue
    q->rear = (q->rear + 1) % q->capacity;
    q->buffer[q->rear] = data;
    q->size++;
    
    // Signal that the queue is not empty
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

void *dequeue(queue_t q) {
    if (!q) {
        return NULL;
    }
    
    pthread_mutex_lock(&q->lock);
    
    // Wait until there's an element in the queue or the queue is shutdown
    while (q->size == 0 && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    
    // If the queue is empty and shutdown, return NULL
    if (q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    
    // Remove the element from the queue
    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    
    // Signal that the queue is not full
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    
    return data;
}

void queue_shutdown(queue_t q) {
    if (!q) {
        return;
    }
    
    pthread_mutex_lock(&q->lock);
    q->is_shutdown_flag = true;
    
    // Wake up all waiting threads so they can check the shutdown flag
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

bool is_empty(queue_t q) {
    if (!q) {
        return true;
    }
    
    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);
    
    return empty;
}

bool is_shutdown(queue_t q) {
    if (!q) {
        return true;
    }
    
    pthread_mutex_lock(&q->lock);
    bool shutdown = q->is_shutdown_flag;
    pthread_mutex_unlock(&q->lock);
    
    return shutdown;
}
