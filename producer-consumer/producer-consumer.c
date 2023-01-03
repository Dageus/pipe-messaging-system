#include "producer-consumer.h"
#include <stdlib.h>
#include <stdio.h>

//declare functions from producer-consumer.h
int pcq_create(pc_queue_t *queue, size_t capacity);
int pcq_destroy(pc_queue_t *queue);
int pcq_enqueue(pc_queue_t *queue, void *elem);
void *pcq_dequeue(pc_queue_t *queue);

// new functions
void subscriber_message(const char* message) {
    fprintf(stdout, "%s\n", message);
}

void list_boxes(const char* box_name, size_t box_size, size_t n_publishers, size_t n_subscribers){
    fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers, n_subscribers);
}

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