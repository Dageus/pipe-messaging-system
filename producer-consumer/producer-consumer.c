#include "producer-consumer.h"
#include <stdlib.h>
#include <stdio.h>

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