#include "producer-consumer.h"
#include <stdlib.h>
#include <stdio.h>

/*
    ->Create the queue using the pcq_create function, and set its capacity.

    ->Create a number of threads using the pthread_create function, 
    passing in a function that will run when the thread is started. 
    This function will typically include a loop that calls pcq_dequeue to 
    remove elements from the queue and process them.

    ->When a new client connects to the server, 
    create a new thread and assign it to a function that will handle the client's requests. 
    This function will typically include a loop that calls pcq_enqueue to
    add elements to the queue for the other threads to process.

    ->Use the pthread_join function in the main thread to wait for the worker threads to 
    complete their tasks before the main thread exits.

    ->Use the pcq_destroy function to release the resources of the queue when the server is shutting down.
*/

// functions from producer-consumer.h
int pcq_create(pc_queue_t *queue, size_t capacity) {
    pc_queue_t* queue = (pc_queue_t*) malloc(sizeof(pc_queue_t));
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;
    queue->pcq_buffer = (void**) malloc(sizeof(void*) * capacity);
    queue->pcq_current_size_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    queue->pcq_head_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    queue->pcq_tail_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    queue->pcq_pusher_condvar_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    queue->pcq_popper_condvar_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    queue->pcq_pusher_condvar = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    queue->pcq_popper_condvar = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    return 0;

}

int pcq_destroy(pc_queue_t *queue) {
    // release the internal resources of the queue
    free(queue->pcq_buffer);
    queue->pcq_buffer = NULL;
    queue->pcq_capacity = 0;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;
    pthread_mutex_destroy(&queue->pcq_current_size_lock);
    pthread_mutex_destroy(&queue->pcq_head_lock);
    pthread_mutex_destroy(&queue->pcq_tail_lock);
    pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
    pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
    pthread_cond_destroy(&queue->pcq_pusher_condvar);
    pthread_cond_destroy(&queue->pcq_popper_condvar);
    return 0;

}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    // insert a new element at the front of the queue
    // If the queue is full, sleep until the queue has space
    pthread_mutex_lock(&queue->pcq_pusher_condvar_lock);
    while (queue->pcq_current_size == queue->pcq_capacity) {
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock);
    }
    pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock);
    // insert the element
    pthread_mutex_lock(&queue->pcq_tail_lock);
    queue->pcq_buffer[queue->pcq_tail] = elem;
    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
    pthread_mutex_unlock(&queue->pcq_tail_lock);

    // broadcast/signal to the popper that he can pop
    pthread_mutex_lock(&queue->pcq_pusher_condvar_lock);
    pthread_cond_signal(&queue->pcq_pusher_condvar);
    pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock);

    queue->pcq_current_size++;
    return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
    // remove and return the element at the back of the queue
    // If the queue is empty, sleep until the queue has an element
    pthread_mutex_lock(&queue->pcq_popper_condvar_lock);
    while (queue->pcq_current_size == 0) {
        pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_popper_condvar_lock);
    }
    pthread_mutex_unlock(&queue->pcq_popper_condvar_lock);

    // remove the element
    pthread_mutex_lock(&queue->pcq_head_lock);
    void* elem = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity; // what is this doing here
    pthread_mutex_unlock(&queue->pcq_head_lock);

    // broadcast/signal to the pusher that he can push
    pthread_mutex_lock(&queue->pcq_popper_condvar_lock);
    pthread_cond_signal(&queue->pcq_popper_condvar);
    pthread_mutex_unlock(&queue->pcq_popper_condvar_lock);

    queue->pcq_current_size--;
    return elem;
    // IMPORTANT
}