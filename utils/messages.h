#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>

#define SUBCRIBER_MESSAGE (const char*)     \
    fprintf(stdout, "%s\n", message);

#define LIST_BOXES_MESSAGE (const char*)    \
    fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers, n_subscribers);

#define NO_BOXES_FOUND ()                   \
    fprintf(stdout, "NO BOXES FOUND\n");

#define SUCCESFUL_BOX_RESPONSE ()           \
    fprintf(stdout, "OK\n");

#define ERROR_BOX_RESPONSE (const char*)    \
    fprintf(stdout, "ERROR %s\n", error_message);


void subscriber_message(const char* message) {
    fprintf(stdout, "%s\n", message);
}

void list_boxes_message(const char* box_name, size_t box_size, size_t n_publishers, size_t n_subscribers){
    fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers, n_subscribers);
}

void no_boxes_found() {
    fprintf(stdout, "NO BOXES FOUND\n");
}

void succesful_box_response() {
    fprintf(stdout, "OK\n");
}

void error_box_response(const char* error_message) {
    fprintf(stdout, "ERROR %s\n", error_message);
}

#endif  // MESSAGES_H