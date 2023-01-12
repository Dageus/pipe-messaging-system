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
int send_message(int pipe_fd, char const *str) {
    size_t len = strlen(str);
    size_t written = 0;

    while (written < len) {
        size_t to_write =  len - written;
        ssize_t ret = write(pipe_fd, str + written, to_write);
        if (ret < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            return -1;
        }

        written += (size_t) ret;
    }

    // check if the message includes a '\n' 

    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int sign_in(char *register_pipe_name, char *pipe_name, char *box_name) {
    int pipe_fd = open(register_pipe_name, O_WRONLY);
    if (pipe_fd < 0) { // error
        return -1;
    }

    char message[256];
    sprintf(message, "pub %s %s", pipe_name, box_name);
    if (write(pipe_fd, message, strlen(message)) < 0) { // error
        return -1;
    }

    close(pipe_fd);

    return 0;
}

/*
 * format:
 *  - pub <register_pipe_name> <pipe_name> <box_name>
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    char *register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the publisher wants to connect to
    char *pipe_name = argv[2];          // pipe_name is the name of the pipe to which the publisher wants to write to
    char *box_name = argv[3];           // box_name is the name of the box to which the publisher wants to publish to

    if (mkfifo(pipe_name, 0666) == -1) {
        fprintf(stderr, "failed: could not create pipe: %s\n", pipe_name);
        return -1;
    }

    if (sign_in(register_pipe_name, pipe_name, box_name) < 0) {
        fprintf(stderr, "failed: could not sign in to box\n");
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

    return 0;
}
