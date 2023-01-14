#ifndef MANAGER_H
#define MANAGER_H

#include "logging.h"
#include "structures.h"
#include "messages.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>

// function to process commands and responses from the message broker
int process_command(int pipe_fd, u_int8_t code);


// function to read input from the pipe
int read_pipe_input(int pipe_fd, fd_set read_fds);

// function to create a message
char* create_message(u_int8_t code, char* client_pipe_name, char* box_name, unsigned int size_of_message);

// Function that leasts all the boxes
int list_boxes_request(char* register_pipe_name, char* named_pipe);

// Function that sends a create box request
int create_box_request(char* register_pipe_name, char *named_pipe, char *box_name);

// Function that sends a remove box request
int remove_box(char* register_pipe_name, char *named_pipe, char *box_name);

int main(int argc, char **argv);

#endif
