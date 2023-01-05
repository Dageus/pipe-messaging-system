#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>

void subscriber_message(const char* message) {
    fprintf(stdout, "%s\n", message);
}

void list_boxes(const char* box_name, size_t box_size, size_t n_publishers, size_t n_subscribers){
    fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers, n_subscribers);
}

void no_boxes_found() {
    fprintf(stdout, "NO BOXES FOUND\n");
}

void subscriber_message(const char* message) {
    fprintf(stdout, "%s\n", message);
}

void succesful_box_response() {
    fprintf(stdout, "OK\n");
}

void error_box_response(const char* error_message) {
    fprintf(stdout, "ERROR %s\n", error_message);
}

#endif