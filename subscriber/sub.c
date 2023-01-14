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


/*
 * - the subscriber connects to the mbroker, citing the box name it wants to subscribe to
 *
 * - the mbroker checks if the box exists
 * 
 * - the subscriber scrapes the existing messages from the box and prints them to stdout
 * 
 * - the subscriber then waits for new messages to be published to the box
 * 
 * - prints new messages received to the box to the stdout
 * 
 * - to finish the session, the subscriber must receive a SIGINT (Ctrl+C), then closing the session and printing the number of messages received
 * 
 * - the named pipe is chosen by the subscriber
 * 
 * - the named pipe must be removed when the subscriber exits
 *
*/

int messages_received = 0;  // number of messages received by the subscriber
volatile sig_atomic_t stop = 0;

void sigint_handler(int signum) {
    (void)signum;
    fprintf(stderr, "[INFO]: received SIGINT\n");
    stop = 1;
}

int read_pipe_input(int pipe_fd, fd_set read_fds) {
    int sel = select(pipe_fd + 1, &read_fds, NULL, NULL, NULL);
    if (sel < 0) {
        return -1;
    } else {
        // data is available on the named pipe
        // read the data from the pipe
        u_int8_t code;
        ssize_t num_bytes = read(pipe_fd, &code, sizeof(code));
        if (num_bytes == 0) {
            // num_bytes == 0 indicates EOF
            fprintf(stderr, "[INFO]: pipe closed\n");
            return 0;
        } else if (num_bytes == -1) {
            // num_bytes == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        fprintf(stderr, "[INFO]: received %zd B\n", num_bytes);
        fprintf(stdout, "[INFO]: code: %d\n", code);

        if (code == 10) {
            // parse the message
            char message[1024];
            num_bytes = read(pipe_fd, message, sizeof(message));
            if (num_bytes == 0) {
                // num_bytes == 0 indicates EOF
                fprintf(stderr, "[INFO]: pipe closed\n");
                return 0;
            } else if (num_bytes == -1) {
                // num_bytes == -1 indicates error
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            fprintf(stdout, "[INFO]: message: %s\n", message);
            messages_received++;
        } else {
            fprintf(stderr, "[ERR]: unknown code: %d\n", code);
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

    u_int8_t code = 2;
    fprintf(stderr, "[INFO]: signing to box: %s\n", box_name);
    char* message = create_message(code, pipe_name, box_name);
    fprintf(stderr, "[INFO]: sending message: %s with len %ld\n", message, strlen(message));
    if (write(pipe_fd, message, 289) < 0) { // error
        return -1;
    }

    if (close(pipe_fd) < 0) { // error
        return -1;
    }

    return 0;
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

    // unlink register_pipe_name if it already exists
    if (unlink(pipe_name) < 0 && errno != ENOENT) {
        return -1;
    }

    fprintf(stderr, "[INFO]: creating named pipe: %s\n", pipe_name);


    if (mkfifo(pipe_name, 0666) < 0) {
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

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    signal(SIGINT, sigint_handler); // register the SIGINT handler
    
    while (!stop) {
        // wait for data to be available on the named pipe
        if (read_pipe_input(pipe_fd, read_fds) < 0) {
            fprintf(stderr, "failed: could not check for pipe input\n");
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

    // print the number of messages received
    fprintf(stdout, "received %d messages\n", messages_received);

    return -1;
}