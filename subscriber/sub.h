#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "logging.h"
#include "structures.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>

//Functions

//Function that ends the session
void sigint_handler(int signum);

//Function that reads the pipe input
int read_pipe_input(int pipe_fd);

//Function that creates a message
char* create_message(u_int8_t code, char* pipe_name, char const *box_name);

//Function that asks the broker to create a box
int sign_in(char *register_pipe_name, char* pipe_name, char *box_name);

void close_subscriber(int exit_status);

int main(int argc, char **argv);

#endif