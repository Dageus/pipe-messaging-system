#ifndef PUBLISHER_H
#define PUBLISHER_H

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

//Function that sends a message to the broker
int send_message(int pipe_fd, char const *message_written);

//Function that creates a message
char* create_message(u_int8_t code, char const *pipe_name, char const *box_name);

//Function that asks the broker to create a box
int sign_in(char *register_pipe_name, char *pipe_name, char *box_name);

int main(int argc, char **argv);


#endif