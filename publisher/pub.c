#include "logging.h"
#include "structures.h"
#include "messages.h"
#include "pub.h"
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

int send_message(int pipe_fd, char const *message_written) {
    u_int8_t code = OP_CODE_PUBLISHER_MESSAGE;
    char* message = malloc(sizeof(char) * 1025);
    memcpy(message, &code, sizeof(u_int8_t));
    memcpy(message + sizeof(u_int8_t), message_written, strlen(message_written));
    memset(message + sizeof(u_int8_t) + strlen(message_written), '\0', 1024 - strlen(message_written));

    if (write(pipe_fd, message, 1025) < 0) { // error
        fprintf(stderr, "failed: could not write to pipe\n");
        return -1;
    }

    return 0;
}

char* create_message(u_int8_t code, char const *pipe_name, char const *box_name) {
    char* message = malloc(sizeof(char) * 289);
    memcpy(message, &code, sizeof(u_int8_t));
    memcpy(message + sizeof(u_int8_t), pipe_name, strlen(pipe_name));
    memset(message + sizeof(u_int8_t) + strlen(pipe_name), '\0', 256 - strlen(pipe_name));
    memcpy(message + 257, box_name, strlen(box_name));
    memset(message + 257 + strlen(box_name), '\0', 32 - strlen(box_name));
    return message;
}

int sign_in(char *register_pipe_name, char *pipe_name, char *box_name) {
    int pipe_fd = open(register_pipe_name, O_WRONLY);
    if (pipe_fd < 0) { // error
        fprintf(stderr, "failed: could not open register pipe\n");
        return -1;
    }

    u_int8_t code = OP_CODE_REGISTER_PUBLISHER;
    char* message = create_message(code, pipe_name, box_name);
    if (write(pipe_fd, message, 289) < 0) { // error
        fprintf(stderr, "failed: could not write to register pipe\n");
        return -1;
    }
    
    if (close(pipe_fd) < 0) { // error
        fprintf(stderr, "failed: could not close register pipe\n");
        return -1;
    }

    return 0;
}

int check_args(char *register_pipe_name, char *pipe_name, char *box_name) {
    if (register_pipe_name == NULL || pipe_name == NULL || box_name == NULL) {
        fprintf(stderr, "failed: one or more of the arguments is NULL\n");
        return -1;
    }
    if (strlen(register_pipe_name) > MAX_NAMED_PIPE_SIZE) {
        fprintf(stderr, "failed: register_pipe_name is too long\n");
        return -1;
    }

    if (strlen(pipe_name) > MAX_NAMED_PIPE_SIZE) {
        fprintf(stderr, "failed: pipe_name is too long\n");
        return -1;
    }

    if (strlen(box_name) > MAX_BOX_NAME_SIZE) {
        fprintf(stderr, "failed: box_name is too long\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    char *register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the publisher wants to connect to
    char *pipe_name = argv[2];          // pipe_name is the name of the pipe to which the publisher wants to write to
    char *box_name = argv[3];           // box_name is the name of the box to which the publisher wants to publish to

    if (check_args(register_pipe_name, pipe_name, box_name) < 0) {
        return -1;
    }

    // unlink register_pipe_name if it already exists
    if (unlink(pipe_name) < 0 && errno != ENOENT) {
        return -1;
    }

    if (mkfifo(pipe_name, 0640) < 0) {
        fprintf(stderr, "failed: could not create pipe: %s\n", pipe_name);
        return -1;
    }

    if (sign_in(register_pipe_name, pipe_name, box_name) < 0) {
        fprintf(stderr, "failed: could not sign in to box\n");
        return -1;
    }

    int pipe_fd = open(pipe_name, O_WRONLY);
    if (pipe_fd < 0) {
        return -1;
    }

    while (true) {
        // wait for input from user in stdin
        char message[1024];
        if (fgets(message, sizeof(message), stdin) == NULL) {
            // received an EOF, exit
            break;
        }

        message[strcspn(message, "\n")] = 0;

        if (strlen(message) < MAX_MESSAGE_SIZE) {
            // fill the rest of the message with '\0'
            memset(message + strlen(message), '\0', MAX_MESSAGE_SIZE - strlen(message));
        }

        if (send_message(pipe_fd, message) < 0) {
            fprintf(stderr, "failed: could not send message\n");
            return -1;
        }
    }

    // close the named pipe
    close(pipe_fd);

    // remove the named pipe
    if (unlink(pipe_name) < 0) {
        fprintf(stderr, "failed: could not remove pipe: %s\n", pipe_name);
        return -1;
    }

    return 0;
}