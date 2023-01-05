#include "producer-consumer.h"
#include <stdlib.h>
#include <stdio.h>

//declare functions from producer-consumer.h
int pcq_create(pc_queue_t *queue, size_t capacity);
int pcq_destroy(pc_queue_t *queue);
int pcq_enqueue(pc_queue_t *queue, void *elem);
void *pcq_dequeue(pc_queue_t *queue);

// functions from producer-consumer.h
int pcq_create(pc_queue_t *queue, size_t capacity) {

}

int pcq_destroy(pc_queue_t *queue) {

}

int pcq_enqueue(pc_queue_t *queue, void *elem) {

}

void *pcq_dequeue(pc_queue_t *queue) {

}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "usage: producer-consumer <pipename>\n");
    WARN("unimplemented"); // TODO: implement
    return -1;
}