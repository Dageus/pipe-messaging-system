#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <errno.h>

#define TIMEOUT_SECS 10

/*

returns 0 on success, -1 on failure
*/
int sub_to_box(char *register_pipe_name, char *box_name) {
// subscribe to the specified box in the mbroker

// send the subscribe command and the box name to the mbroker through the named pipe

// receive messages from the mbroker through the named pipe
return 0;
}

int main(int argc, char **argv) {
if (argc != 3) {
fprintf(stderr, "failed: not enough arguments\n");
return -1;
}

// open the named pipe for reading
int pipe_fd = open(argv[1], O_RDONLY);
if (pipe_fd < 0) {
    perror("open");
    return -1;
}

// set up the file descriptor set for the select function
fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(STDIN_FILENO, &read_fds); // wait for input from stdin
FD_SET(pipe_fd, &read_fds); // wait for data to be available on the named pipe

// set up the timeout for the select function
struct timeval timeout;
timeout.tv_sec = TIMEOUT_SECS;
timeout.tv_usec = 0;

// wait for data to be available on stdin or the named pipe
int ret = select(pipe_fd + 1, &read_fds, NULL, NULL, &timeout);
if (ret < 0) {
    perror("select");
    return -1;
} else if (ret == 0) {
    // the select function timed out
    fprintf(stderr, "select timed out\n");
    return -1;
} else {
    // check which file descriptors have data available
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
        // data is available on stdin
        // read the data from stdin
        char buffer[1024];
        ssize_t num_bytes = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (num_bytes < 0) {
            perror("read");
            return -1;
        } else {
             // process the data that was read from the pipe
        }
    }
}

// close the named pipe
close(pipe_fd);

return 0;
}