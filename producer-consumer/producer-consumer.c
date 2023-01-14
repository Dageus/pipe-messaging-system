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
    (void)queue; // suppress unused parameter warning
    (void)capacity; // suppress unused parameter warning
    return 0;

}

int pcq_destroy(pc_queue_t *queue) {
    (void)queue; // suppress unused parameter warning
    return 0;

}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    (void)queue; // suppress unused parameter warning
    (void)elem; // suppress unused parameter warning
    return 0;


}

void *pcq_dequeue(pc_queue_t *queue) {
    (void)queue; // suppress unused parameter warning
    return NULL;


}