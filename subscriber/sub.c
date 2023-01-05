#include "logging.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


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


/*
 * returns 0 on success, -1 on failure
 *
*/
int sub_to_box(char* named_pipe, char* box_name) {
    // open box_name
    // read from box_name
    // close box_name
    return 0;
}


/*
 * format:
 *  - sub <register_pipe_name> <box_name>
*/
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }
 
   // use select here to wait for input from stdin and the named pipe

    char* register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the subscriber wants to connect to
    char* box_name = argv[2];           // box_name is the name of the box to which the subscriber wants to subscribe to
    
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