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

mbroker_t *mbroker;

box_t *box;

// array to keep track of the sessions

// array to keep track of the pipes

// array to keep track of the boxes

// IMPORTANT: 
// - functions to implement yet:

int find_box();



// completed code: 

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

        if (process_command(pipe_fd, read_fds, code) < 0) {
            return -1;
        }

    }
}

int process_command(int pipe_fd, fd_set read_fds, u_int8_t code) {
    // read the message from the pipe
    switch (code)
    {
    case 1:         // register a publisher
        char message[1024];
        ssize_t num_bytes = read(pipe_fd, message, sizeof(message));
        if (num_bytes < 0) {
            perror("read");
            return -1;
        }
        break;
    case 2:                 // register a subscriber
        /* code */
        break;
    case 3:                 // create a box
        /* code */
        break;
    case 5:                 // remove a box
        /* code */
        break;
    case 7:                 // list boxes
        /* code */
        break;
    case 9:                 // publisher sends a message
        /* code */
        break;
    default:                // invalid command
        return -1;
    }
}

int answer_to_pipe(u_int8_t code,    char* client_named_pipe_path){
    // write to pipe to answer to client
}

int create_box(char *box_name) {
    // check if box already exists

    // create box
    int box_fd = tfs_open(box_name, O_CREAT);

    if (box_fd < 0) {
        fprintf(stderr, "failed: could not create box %s", box_name);
        return -1;
    }

    box = (box_t*) malloc(sizeof(box_t));

    box->box_fd = box_fd;
    box->box_name = box_name;

    return 0;
}

int sub_to_box(char *box_name) {
    // check if box exists



    return 0;
}

int pub_to_box(char *box_name) {
    // check if box exists

    // check if there is already a publisher
    if (box->publisher != NULL) {
        fprintf(stderr, "failed: box %s already has a publisher", box_name);
        return -1;
    }

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

    mbroker = (mbroker_t*) malloc(sizeof(mbroker_t));

    mbroker->register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the manager wants to connect to
    mbroker->max_sessions = atoi(argv[2]);       // max_sessions is the maximum number of sessions that can be open at the same time
    
    // unlink register_pipe_name if it already exists
    if (unlink(mbroker->register_pipe_name) < 0) {
        perror("unlink");
        return -1;
    }
    // create the named pipe
    if (mkfifo(mbroker->register_pipe_name, 0666) < 0) {
        perror("mkfifo");
        return -1;
    }

    int pipe_fd = open(mbroker->register_pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
        perror("open");
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    while (true) {
        // wait for data to be available on the named pipe
        
        if (read_pipe_input(pipe_fd, read_fds) < 0) {
            fprintf(stderr, "failed: could not check for pipe input\n");
            return -1;
        }
        if (read_pipe_input(pipe_fd, read_fds) == 5){
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
