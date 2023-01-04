#include "logging.h"
#include "structures.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * a session remains open until either:
 *      - the publisher or subscriber closes the pipe
 *      - a box is removed from the manager 
 */

// register_pipe_name is the name of the pipe that mbroker controls

/*
 * format:
 *  - mbroker <register_pipe_name> <max_sessions>
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    // use
    // while (true) {
        // use select() to wait for input from the register pipe
        // if input is received, create a new session or box or whatever
        

    char *register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the manager wants to connect to
    int max_sessions = atoi(argv[2]);   // max_sessions is the maximum number of sessions that can be open at the same time

    // more functions might be needed
    

    WARN("unimplemented"); // TODO: implement
    return -1;
}
