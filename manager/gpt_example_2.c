#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>


/*
returns 0 on success, -1 on failure
*/
int create_box(char *box_name) {
    char client_pipe_name[256];
    
    // create the named pipe
    if (mkfifo(client_pipe_name, 0666) < 0) {
        perror("mkfifo");
        return -1;
    }

    // open the named pipe for writing
    int pipe_fd = open(client_pipe_name, O_WRONLY);
    if (pipe_fd < 0) {
        perror("open");
        return -1;
    }

    // send the create command and the box name to the mbroker through the pipe
    char message[256];
    sprintf(message, "create %s", box_name);
    if (write(pipe_fd, message, strlen(message)) < 0) {
        perror("write");
        return -1;
    }

    // close the pipe
    close(pipe_fd);

    // remove the pipe
    if (unlink(client_pipe_name) < 0) {
        perror("unlink");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    char *register_pipe_name = argv[1]; // unused in this example
    char *command = argv[2];
    if (strcmp(command, "create") == 0 && argc > 3) {
        char *box_name = argv[3];
        if (create_box(box_name) < 0) {
            fprintf(stderr, "failed to create box %s\n", box_name);
            return -1;
        }
    } else {
        fprintf(stderr, "invalid command\n");
        return -1;
    }

    return 0;
}