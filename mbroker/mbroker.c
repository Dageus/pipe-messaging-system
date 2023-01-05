#include "logging.h"
#include "structures.h"
#include "messages.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>

/*
 * a session remains open until either:
 *      - the publisher or subscriber closes the pipe
 *      - a box is removed from the manager 
 */

// register_pipe_name is the name of the pipe that mbroker controls

int current_sessions = 0;   // number of sessions currently open
int max_sessions;           // maximum number of sessions that can be open at the same time

// array to keep track of the sessions

// array to keep track of the pipes

// array to keep track of the boxes

int check_for_pipe_input(int pipe_fd, fd_set read_fds) {
    int ret = select(pipe_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
            return -1;
        } else {
            // data is available on the named pipe
            // read the data from the pipe
            char buffer[1024];
            ssize_t num_bytes = read(pipe_fd, buffer, sizeof(buffer));
            if (num_bytes < 0) {
                perror("read");
                return -1;
            } else {
                // process the data that was read from the pipe
            }
        }
}

/*
 * format:
 *  - mbroker <register_pipe_name> <max_sessions>
 */
int main(int argc, char **argv) {

    // initialize tfs in mbroker

    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    char *register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the manager wants to connect to
    max_sessions = atoi(argv[2]);       // max_sessions is the maximum number of sessions that can be open at the same time

    int pipe_fd = open("/path/to/named_pipe", O_RDONLY);
    if (pipe_fd < 0) {
        perror("open");
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    while (true) {
        // wait for data to be available on the named pipe
        
        if (check_for_pipe_input(pipe_fd, read_fds) < 0) {
            fprintf(stderr, "failed: could not check for pipe input\n");
            return -1;
        }
 
        // if input is a message, process the message

        // if input is an EOF, close the session
    
    }

    WARN("unimplemented"); // TODO: implement
    return -1;
}
