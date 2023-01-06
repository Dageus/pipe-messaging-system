#include "logging.h"
#include "structures.h"
#include "messages.h"
#include "operations.h"
#include "state.h"
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
            char buffer[MAX_MESSAGE_SIZE];
            ssize_t num_bytes = read(pipe_fd, buffer, sizeof(buffer));
            if (num_bytes == 0) {
                // num_bytes == 0 indicates EOF
                fprintf(stderr, "[INFO]: pipe closed\n");
                return 0;
            } else if (num_bytes == -1) {
                // num_bytes == -1 indicates error
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
}

int create_box(char *box_name) {
    // check if box already exists
    if (box_exists(box_name)) {
        fprintf(stderr, "failed: box already exists\n");
        return -1;
    }

    // create box
    if (create_new_box(box_name) < 0) {
        fprintf(stderr, "failed: could not create box\n");
        return -1;
    }

    tfs_open(box_name, O_CREAT); // how to create box

    return 0;
}

/*
 * format:
 *  - mbroker <register_pipe_name> <max_sessions>
 */
int main(int argc, char **argv) {

    if (tfs_init(NULL) < 0) {
        fprintf(stderr, "failed: could not initialize tfs\n");
        return -1;
    }

    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    char *register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the manager wants to connect to
    max_sessions = atoi(argv[2]);       // max_sessions is the maximum number of sessions that can be open at the same time


    // unlink register_pipe_name if it already exists
    if (unlink(register_pipe_name) < 0) {
        perror("unlink");
        return -1;
    }
    // create the named pipe
    if (mkfifo(register_pipe_name, 0666) < 0) {
        perror("mkfifo");
        return -1;
    }

    int pipe_fd = open(register_pipe_name, O_RDONLY);
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
        if (check_for_pipe_input(pipe_fd, read_fds) == 5){
            // close the session
            return 0;
        }
    
    }

    if (tfs_destroy() < 0) {
        fprintf(stderr, "failed: could not destroy tfs\n");
        return -1;
    }

    WARN("unimplemented"); // TODO: implement
    return -1;
}
