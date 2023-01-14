#ifndef MBROKER_H
#define MBROKER_H

#include "logging.h"
#include "structures.h"
#include "messages.h"
#include "operations.h"
#include "state.h"
#include "producer-consumer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

// Variables 
mbroker_t *mbroker;

box_list_t *box_list;

pc_queue_t *pc_queue;

pthread_t *thread_array;

// Functions

//Function that ends the session
int end_session();

//Function that gets the box by the publisher pipe name
box_t* get_box_by_publisher_pipe(char* publisher_pipe_name);

//Function that verifies if the box is in the list
int box_in_list(char* box_name);

//Functiion that creates a message
char* create_message(u_int8_t code, char* message_to_write);

//Function that sends a message to the subscribers
int spread_message(char* message_to_write, char* publisher_pipe_name);

//Function that finds the box by the name
box_t* find_box_by_name(char* box_name);

//Function that creates an aswer message
char* create_answer(u_int8_t code, int32_t return_code, char* error_message, unsigned int size_of_answer);

//Function that creates a box
int create_box_command(char* box_name);

//Function that removes a box
int remove_box_command(char* box_name);

//Function that registers a publisher
int register_publisher_command(char* client_named_pipe_path, char* box_name);

//Function that registers a subscriber
int register_subscriber_command(char* client_named_pipe_path, char* box_name);

//Function that creates a listing message
char* create_listing_message(box_list_t* box_node);

//Function that lists all the boxes
int list_boxes_command(char* manager_pipe_name);

//Function that checks if the box exists
int pub_to_box(char *box_name);

//Function that writes to a box
int write_to_box(char* box_name, char* message, size_t message_size);

//Function that reads from a box
int read_from_box(char* box_name, char* message, size_t message_size);

//Function that reads from the pipe
int read_publisher_pipe_input(int pipe_fd, fd_set read_fds, char* publisher_named_pipe, char* box_name);

//Function that listens to the publisher
int listen_to_publisher(char* publisher_named_pipe, char* box_name);

//Function that manages the commands
int process_command(int pipe_fd, u_int8_t code);

int main(int argc, char **argv);




#endif