#include "producer-consumer.h"
#include "structures.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

// functions from producer-consumer.h
int pcq_create(pc_queue_t *queue, size_t capacity) {
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;
    queue->pcq_buffer = (void**) malloc(sizeof(void*) * capacity);
    if (queue->pcq_buffer == NULL) {
        return -1;
    }
    pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
    pthread_mutex_init(&queue->pcq_head_lock, NULL);
    pthread_mutex_init(&queue->pcq_tail_lock, NULL);
    pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);
    pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_pusher_condvar, NULL);
    pthread_cond_init(&queue->pcq_popper_condvar, NULL);
    return 0;
}

int pcq_destroy(pc_queue_t *queue) {
    // release the internal resources of the queue
    if (queue->pcq_buffer == NULL) {
        return -1;
    }
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
    if (pthread_mutex_lock(&queue->pcq_pusher_condvar_lock) != 0) {
        return -1;
    }
    while (queue->pcq_current_size == queue->pcq_capacity) {
        if (pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock) != 0) {
            return -1;
        }
    }
    if (pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock) != 0) {
        return -1;
    }
    // insert the element
    if (pthread_mutex_lock(&queue->pcq_tail_lock) != 0) {
        return -1;
    }
    session_t *session;
    memcpy(&session, elem, sizeof(session_t*)); // should've put pointer in elem
    queue->pcq_buffer[queue->pcq_tail] = elem;
    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
    if (pthread_mutex_lock(&queue->pcq_current_size_lock) != 0) {
        return -1;
    }
    queue->pcq_current_size++;
    if (pthread_mutex_unlock(&queue->pcq_current_size_lock) != 0) {
        return -1;
    }
    if (pthread_mutex_unlock(&queue->pcq_tail_lock) != 0) {
        return -1;
    }
    if (pthread_mutex_lock(&queue->pcq_popper_condvar_lock) != 0) {
        return -1;
    }
    // broadcast/signal to the popper that he can po
    if (pthread_cond_signal(&queue->pcq_popper_condvar) != 0) {
        return -1;
    }
    if (pthread_mutex_unlock(&queue->pcq_popper_condvar_lock) != 0) {
        return -1;
    }
    return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
    // remove and return the element at the back of the queue
    // If the queue is empty, sleep until the queue has an element
    if (pthread_mutex_lock(&queue->pcq_popper_condvar_lock) != 0){
        return NULL;
    }
    
    while (queue->pcq_current_size == 0) {
        if (pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_popper_condvar_lock) != 0) {
            return NULL;
        }
    }
    if (pthread_mutex_unlock(&queue->pcq_popper_condvar_lock) != 0){
        return NULL;
    }

    // remove the element
    if (pthread_mutex_lock(&queue->pcq_head_lock) != 0){
        return NULL;
    }
    void* elem = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
    if (pthread_mutex_lock(&queue->pcq_current_size_lock) != 0){
        return NULL;
    }
    queue->pcq_current_size--;
    if (pthread_mutex_unlock(&queue->pcq_current_size_lock) != 0){
        return NULL;
    }

    if (pthread_mutex_unlock(&queue->pcq_head_lock) != 0){
        return NULL;
    }
    if (pthread_mutex_lock(&queue->pcq_pusher_condvar_lock) != 0){
        return NULL;
    }
    // broadcast/signal to the pusher that he can push
    if (pthread_cond_signal(&queue->pcq_pusher_condvar) != 0){
        return NULL;
    }
    if (pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock) != 0){
        return NULL;
    }
    return elem;
}