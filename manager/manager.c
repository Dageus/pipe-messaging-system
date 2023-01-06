#include "logging.h"
#include "structures.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>

// pipe path format: /tmp/client_pipe_<num>

/*
 *
 * - when the manager is launched, it connects to the mbroker
 * 
 * - it receives the answer from the mbroker in the SAME PIPE that the manager used to connect to the mbroker
 * 
 * - it prints the answer to the stdout, and then exits
 * 
 * - to create a pipe use the function mkfifo(<path>, <mode>)
 * 
 * - to use the pipe use the function open(<path>, <flags>)
 * 
 * - to remove a pipe use the function unlink(<path>)
 */

// manager creates named pipe for register

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

/*
 * returns 0 on success, -1 on failure
 */
int list_boxes(char* named_pipe) {
    // list boxes in mbroker

    // open the pipe
    int pipe_fd = open(named_pipe, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", named_pipe);
        return -1;
    }
    
    // send the create command and the box name to the mbroker through the pipe
    char *message = create_message(7, named_pipe, NULL, BOX_LIST_MESSAGE_SIZE);

    // send this to the mbroker through the named pipe
    if (write(pipe_fd, message, sizeof(message)) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", named_pipe);
        return -1;
    }

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", named_pipe);
        return -1;
    }

    // receive the answer from the mbroker through the named pipe
    return 0;
}

char* create_message(u_int8_t code, char* client_pipe_name, char* box_name, int size_of_message) {
    char message[size_of_message]; // 8 bits from the u_int8_t + 256 bits from the client_named_pipe_path + 32 bits from the box_name
    memcpy(message, &code, sizeof(code));
    memcpy(message, client_pipe_name, sizeof(client_pipe_name));
    if (box_name != NULL)
        memcpy(message, box_name, sizeof(box_name));
    return message;
}

/*
 * returns 0 on success, -1 on failure
*/
int create_box_request(char *named_pipe, char *box_name) {
    // use memcpy to copy the CLIENT_PIPE_PATH + client_pipe_num to the client_pipe_name

    // open the pipe
    int pipe_fd = open(named_pipe, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", named_pipe);
        return -1;
    }
    
    // send the create command and the box name to the mbroker through the pipe
    char *message = create_message(3, named_pipe, box_name, BOX_MESSAGE_SIZE);

    // send this to the mbroker through the named pipe
    if (write(pipe_fd, message, sizeof(message)) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", named_pipe);
        return -1;
    }

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", named_pipe);
        return -1;
    }

    // receive the answer from the mbroker through the named pipe
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int remove_box(char *named_pipe, char *box_name) {
   // use memcpy to copy the CLIENT_PIPE_PATH + client_pipe_num to the client_pipe_name

    // open the pipe
    int pipe_fd = open(named_pipe, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", named_pipe);
        return -1;
    }
    
    // send the create command and the box name to the mbroker through the pipe
    char *message = create_message(5, named_pipe, box_name, BOX_MESSAGE_SIZE);

    // send this to the mbroker through the named pipe
    if (write(pipe_fd, message, sizeof(message)) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", named_pipe);
        return -1;
    }

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", named_pipe);
        return -1;
    }

    // receive the answer from the mbroker through the named pipe
    return 0;
}


// register pipe name is the name of the pipe to which the manager wants to connect to

// IMPORTANT:
//      CREATE STRUCTS TO STORE ESSENTIAL INFORMATION ABOUT THE MANAGER

/*
 * format: 
 *  - manager <register_pipe_name> <pipe_name> create <box_name>
 *  - manager <register_pipe_name> <pipe_name> remove <box_name>
 *  - manager <register_pipe_name> <pipe_name> list
 */
int main(int argc, char **argv) {
    // IMPORTANT: 
    //  - implemenet the function but include the pipe_name
    if (argc < 4) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }    

    char *register_pipe_name = argv[1]; // assign the register pipe name
    char *pipe_name = argv[2];          // assign the pipe name
    char *command = argv[3];            // assign the command

    if (mkfifo(pipe_name, 0666) == -1) {
        fprintf(stderr, "failed: could not create pipe: %s\n", pipe_name);
        return -1;
    }

    if (argc > 4){                      // this means we're either creating or removing a box
        char *box_name = argv[4];
        if (strcmp(command, "create") == 0) {
            create_box_request(register_pipe_name, box_name);
        } else if (strcmp(command, "remove") == 0) {
            remove_box(register_pipe_name, box_name);
        }
    } else {                            // this means we're listing the boxes
        list_boxes(register_pipe_name);
    }

    int pipe_fd = open(pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", pipe_name);
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    while (true){
         // wait for data to be available on the named pipe
        
        if (check_for_pipe_input(pipe_fd, read_fds) < 0) {
            fprintf(stderr, "failed: could not check for pipe input\n");
            return -1;
        }

        // actual code
    }

    WARN("unimplemented"); // TODO: implement
    return -1;
}