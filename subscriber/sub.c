#include "logging.h"
#include "structures.h"
#include "messages.h"
#include "sub.h"
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
#include <pthread.h>

int messages_received = 0;  // number of messages received by the subscriber
volatile sig_atomic_t stop = 0;

void sigint_handler(int signum) {
    (void)signum;
    stop = 1;
    close_subscriber(EXIT_SUCCESS);
}

int read_pipe_input(int pipe_fd) {

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    int sel = select(pipe_fd + 1, &read_fds, NULL, NULL, NULL);
    if (sel < 0) {
        return -1;
    } else {
        // data is available on the named pipe
        // read the data from the pipe
        char answer[ANSWER_MESSAGE_SIZE];
        ssize_t num_bytes = read(pipe_fd, answer, sizeof(answer));
        if (num_bytes == 0) {
            // num_bytes == 0 indicates EOF
            return 0;
        } else if (num_bytes == -1) {
            // num_bytes == -1 indicates error
            fprintf(stderr, "failed: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // parse the op code
        u_int8_t code;
        memcpy(&code, answer, sizeof(u_int8_t));
        if (code == 10) {
            // parse the message
            char message[1024];
            memcpy(message, answer + sizeof(u_int8_t), 1024);

            subscriber_message(message);
            messages_received++;
        } else {
            fprintf(stderr, "failed: unknown code: %d\n", code);
            return -1;
        }
    }
    return 0;
}

char* create_message(u_int8_t code, char* pipe_name, char const *box_name) {
    char* message = malloc(sizeof(char) * 289);
    memcpy(message, &code, sizeof(u_int8_t));
    memcpy(message + sizeof(u_int8_t), pipe_name, strlen(pipe_name));
    memset(message + sizeof(u_int8_t) + strlen(pipe_name), '\0', 256 - strlen(pipe_name));
    memcpy(message + 257, box_name, strlen(box_name));
    memset(message + 257 + strlen(box_name), '\0', 32 - strlen(box_name));
    return message;
}


/*
 * returns 0 on success, -1 on failure
 *
*/
int sign_in(char *register_pipe_name, char* pipe_name, char *box_name) {
    int pipe_fd = open(register_pipe_name, O_WRONLY);
    if (pipe_fd < 0) { // error
        return -1;
    }

    u_int8_t code = OP_CODE_REGISTER_SUBSCRIBER;
    char* message = create_message(code, pipe_name, box_name);
    if (write(pipe_fd, message, 289) < 0) { // error
        return -1;
    }

    if (close(pipe_fd) < 0) { // error
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

void close_subscriber(int exit_code) {
    // print the number of messages received
    fprintf(stderr, "\nreceived %d messages\n", messages_received);
    exit(exit_code);
}

/*
 * format:
 *  - sub <register_pipe_name> <pipe_name> <box_name>
*/
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    char* register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the subscriber wants to connect to
    char* pipe_name = argv[2];          // pipe_name is the name of the pipe to which the subscriber wants to connect to
    char* box_name = argv[3];           // box_name is the name of the box to which the subscriber wants to subscribe to

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
    
    int pipe_fd = open(pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
        return -1;
    }

    signal(SIGINT, sigint_handler); // register the SIGINT handler
    
    while (!stop) {
        // wait for data to be available on the named pipe
        if (read_pipe_input(pipe_fd) < 0) {
            fprintf(stderr, "failed: could not check for pipe input\n");
            return -1;
        }
    }

    // close the named pipe
    if (close(pipe_fd) < 0) {
        fprintf(stderr, "failed: could not close pipe: %s\n", pipe_name);
        return -1;
    }

    // remove the named pipe
    if (unlink(pipe_name) < 0) {
        fprintf(stderr, "failed: could not remove pipe: %s\n", pipe_name);
        return -1;
    }

    close_subscriber(EXIT_SUCCESS);

    return -1;
}

//broadcast para variavel de condicao para que o subscriber possa ler quando o publisher escrever