#include "logging.h"
#include "structures.h"
#include "messages.h"
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

int process_command(int pipe_fd, u_int8_t code) {
    // read the message from the pipe
    switch (code)
    {
    case 4:{                        // response to creation of box
        u_int32_t return_code;
        ssize_t num_bytes;
        num_bytes = read(pipe_fd, &return_code, sizeof(return_code));
        if (num_bytes < 0) {        // error
            return -1;
        }
        if (return_code < 0) {     // box not created
            char error_message[1024];
            num_bytes = read(pipe_fd, &error_message, sizeof(error_message));
            if (num_bytes < 0) {    // error
                return -1;
            }
            // maybe print the message (not sure)
        }
        break;
    }
    case 6:{                        // responde to box removal
        u_int32_t return_code;
        ssize_t num_bytes;
        num_bytes = read(pipe_fd, &return_code, sizeof(return_code));
        if (num_bytes < 0) {        // error
            return -1;
        }
        if (return_code < 0) {     // box not removed
            char error_message[1024];
            num_bytes = read(pipe_fd, &error_message, sizeof(error_message));
            if (num_bytes < 0) {    // error
                return -1;
            }
            // maybe print the message (not sure)
        }
        break;
    }
    case 8:{                        // list boxes
        u_int8_t last;
        last = 0;
        while (last == 0){          // make sure to check if its the last box or not
            // parse the return code
            ssize_t num_bytes;
            num_bytes = read(pipe_fd, &last, sizeof(last));
            if (num_bytes < 0) {    // error
                return -1;
            }
            // parse the box name
            char box_name[32];
            num_bytes = read(pipe_fd, &box_name, sizeof(box_name));
            if (num_bytes < 0) {    // error
                return -1;
            }
            if (strcmp(box_name, "") != 0 && last == 1) { // this means there are no boxes
                break;
            }
            u_int64_t box_size;
            num_bytes = read(pipe_fd, &box_size, sizeof(box_size));
            if (num_bytes < 0) {    // error
                return -1;
            }
            // parse the numer of publishers
            u_int64_t n_publishers;
            num_bytes = read(pipe_fd, &n_publishers, sizeof(n_publishers));
            if (num_bytes < 0) {    // error
                return -1;
            }
            // parse the number of subscribers
            u_int64_t n_subscribers;
            num_bytes = read(pipe_fd, &n_subscribers, sizeof(n_subscribers));
            if (num_bytes < 0) {    // error
                return -1;
            }
            if (strcmp(box_name, "") != 0 && last == 1) { // this means there are no boxes
                no_boxes_found();
            }
        }
        break;
    }                         
    default:                // invalid command
        return -1;
    }
    return 0;
}

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

        if (process_command(pipe_fd, code) < 0) {
            return -1;
        }
        return 1;
    }
    return 0;
}

char* create_message(u_int8_t code, char* client_pipe_name, char* box_name, unsigned int size_of_message) {
    char *message = (char*) malloc(sizeof(char) * size_of_message); // 8 bits from the u_int8_t + 256 bits from the client_named_pipe_path + 32 bits from the box_name
    memcpy(message, &code, sizeof(u_int8_t));
    memcpy(message + 1, client_pipe_name, strlen(client_pipe_name));
    memset(message + strlen(client_pipe_name) + 1, '\0', 256 - strlen(client_pipe_name));
    if (box_name != NULL)
        memcpy(message + 1 + 256, box_name, strlen(box_name));
    return message;
}



/*
 * returns 0 on success, -1 on failure
 */
int list_boxes_request(char* named_pipe) {
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
    fprintf(stderr, "[INFO]: sending request to create box: %s\n", box_name);
    u_int8_t code = 3;
    char *message = create_message(code, named_pipe, box_name, BOX_MESSAGE_SIZE);

    // send this to the mbroker through the named pipe
    if (write(pipe_fd, message, BOX_MESSAGE_SIZE) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", named_pipe);
        return -1;
    }

    fprintf(stderr, "[INFO]: sent request: %s\n[INFO]: closing...\n", message);

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
    if (argc < 4) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }    

    char *register_pipe_name = argv[1]; // assign the register pipe name
    char *pipe_name = argv[2];          // assign the pipe name
    char *command = argv[3];            // assign the command

    fprintf(stderr, "[INFO]: starting manager\n");

    fprintf(stderr, "[INFO]: creating named pipe: %s\n", pipe_name);
    // unlink pipe_name if it already exists
    if (unlink(pipe_name) < 0) {
        fprintf(stderr, "failed: could not unlink pipe: %s\n", pipe_name);
        return -1;
    }

    if (mkfifo(pipe_name, 0666) < 0) {
        fprintf(stderr, "failed: could not create pipe: %s\n", pipe_name);
        return -1;
    } else {
        fprintf(stderr, "[INFO]: named pipe created\n");
    }

    if (argc > 4){                      // this means we're either creating or removing a box
        char *box_name = argv[4];
        if (strcmp(command, "create") == 0) {
            create_box_request(register_pipe_name, box_name);
        } else if (strcmp(command, "remove") == 0) {
            remove_box(register_pipe_name, box_name);
        }
    } else {                            // this means we're listing the boxes
        list_boxes_request(register_pipe_name);
    }

    int pipe_fd = open(pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", pipe_name);
        return -1;
    }
    fprintf(stderr, "opened pipe: %s\n", pipe_name);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    while (true){
        int select_result = read_pipe_input(pipe_fd, read_fds);
        if (select_result == -1) {
            fprintf(stderr, "failed: could not check for pipe input\n");
            return -1;
        } else if (select_result == 1) {                // was successful
            fprintf(stdout, "request was succesful\n");
            break;
        }
        fprintf(stdout, "waiting for input...\n");
        // continue waiting for input
    }
    // close the pipe and exit
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", pipe_name);
        return -1;
    }
    fprintf(stderr, "closed pipe: %s\n", pipe_name);
    return 0;
}