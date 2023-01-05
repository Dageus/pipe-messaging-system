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
 * - publisher asks to join a server in the mbroker, citing the box name to which its gonna write to
 *
 * - mbroker creates a pipe for the publisher to write to
 * 
 * - if the connection is accepted by the mbroker, the client reads from the stdin and writes to the pipe
 * 
 * - each line corresponds to a message 
 * 
 * - it must not include a '\n' character, only the '\0' character
 * 
 * - if the publisher receives a SIGINT, it must close the pipe and exit
 * 
 * - the named pipe must be removed when the publisher exits
 * 
 */

/*
 * returns 0 on success, -1 on failure
*/
int pub_to_box(char* named_pipe, char *box_name) {
    // IMPORTANT 
    // use tfsopen to open the box
    // use tfswrite to write to the box
    // use tfsclose to close the box
    return 0;
}

/*
 * format:
 *  - pub <register_pipe_name> <box_name>
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }


    char *register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the publisher wants to connect to
    char *box_name = argv[2];           // box_name is the name of the box to which the publisher wants to publish to

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
