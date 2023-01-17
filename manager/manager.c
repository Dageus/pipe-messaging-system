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


int process_command(char* answer) {
    // read the message from the pipe
    u_int8_t code;
    memcpy(&code, answer, sizeof(code));
    switch (code)
    {
    case 4:{                        // response to creation of box
        int32_t return_code;
        memcpy(&return_code, answer + sizeof(code), sizeof(return_code));
        if (return_code < 0) {     // box not removed
            char error_message[1024];
            memcpy(error_message, answer + sizeof(code) + sizeof(return_code), sizeof(error_message));
            error_box_response(error_message);
        } else {
            succesful_box_response();
        }
        break;
    }
    case 6:{                        // responde to box removal
        int32_t return_code;
        memcpy(&return_code, answer + sizeof(code), sizeof(return_code));
        if (return_code < 0) {     // box not removed
            char error_message[1024];
            memcpy(error_message, answer + sizeof(code) + sizeof(return_code), sizeof(error_message));
            error_box_response(error_message);
        } else {
            succesful_box_response();
        }
        break;
    }
    case 8:{                        // list boxes
        // parse the last
        size_t last;
        memcpy(&last, answer + sizeof(code), sizeof(last));
        // parse the box name
        char box_name[32];
        memcpy(box_name, answer + sizeof(code) + sizeof(last), sizeof(box_name));

        if (strcmp(box_name, "") == 0 && last == 1) { // this means there are no boxes
            no_boxes_found();
            break;
        }

        size_t box_size;
        memcpy(&box_size, answer + sizeof(code) + sizeof(last) + sizeof(box_name), sizeof(box_size));

        // parse the numer of publishers
        size_t n_publishers;
        memcpy(&n_publishers, answer + sizeof(code) + sizeof(last) + sizeof(box_name) + sizeof(box_size), sizeof(n_publishers));

        // parse the number of subscribers
        size_t n_subscribers;
        memcpy(&n_subscribers, answer + sizeof(code) + sizeof(last) + sizeof(box_name) + sizeof(box_size) + sizeof(n_publishers), sizeof(n_subscribers));

        list_boxes_message(box_name, box_size, n_publishers, n_subscribers);
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
    }
    // data is available on the named pipe
    // read the data from the pipe
    char answer[ANSWER_MESSAGE_SIZE];
    ssize_t num_bytes = read(pipe_fd, answer, sizeof(answer));
    if (num_bytes == 0) {
        // num_bytes == 0 indicates EOF
        return 0;
    } else if (num_bytes == -1) {
        // num_bytes == -1 indicates error
        fprintf(stderr, "failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (process_command(answer) < 0) {
        return -1;
    }
    return 1;
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

int list_boxes_request(char* register_pipe_name, char* named_pipe) {
    // list boxes in mbroker
    
    // open the pipe
    int pipe_fd = open(register_pipe_name, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", register_pipe_name);
        return -1;
    }
    
    // send the create command and the box name to the mbroker through the pipe
    u_int8_t code = OP_CODE_BOX_LIST;
    char *message = create_message(code, named_pipe, NULL, BOX_LIST_MESSAGE_SIZE);

    // send this to the mbroker through the named pipe
    if (write(pipe_fd, message, BOX_LIST_MESSAGE_SIZE) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", register_pipe_name);
        return -1;
    }

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", register_pipe_name);
        return -1;
    }

    // receive the answer from the mbroker through the named pipe
    return 0;
}

int register_box_request(char* register_pipe_name, char *named_pipe, char *box_name) {

    // open the pipe
    int pipe_fd = open(register_pipe_name, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", register_pipe_name);
        return -1;
    }
    
    // send the register command and the box name to the mbroker through the pipe
    u_int8_t code = OP_CODE_REGISTER_BOX;
    char *message = create_message(code, named_pipe, box_name, BOX_MESSAGE_SIZE);

    // send this to the mbroker through the named pipe
    if (write(pipe_fd, message, BOX_MESSAGE_SIZE) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", register_pipe_name);
        return -1;
    }

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", register_pipe_name);
        return -1;
    }

    // receive the answer from the mbroker through the named pipe
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int remove_box(char* register_pipe_name, char *named_pipe, char *box_name) {
    // open the pipe
    int pipe_fd = open(register_pipe_name, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", register_pipe_name);
        return -1;
    }
    
    // send the create command and the box name to the mbroker through the pipe
    char *message = create_message(5, named_pipe, box_name, BOX_MESSAGE_SIZE);

    // send this to the mbroker through the named pipe
    if (write(pipe_fd, message, sizeof(message)) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", register_pipe_name);
        return -1;
    }

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", register_pipe_name);
        return -1;
    }

    // receive the answer from the mbroker through the named pipe
    return 0;
}

int check_args(char* register_pipe_name, char* named_pipe) {
    if (register_pipe_name == NULL) {
        fprintf(stderr, "failed: register pipe name is null\n");
        return -1;
    }
    if (named_pipe == NULL) {
        fprintf(stderr, "failed: named pipe is null\n");
        return -1;
    }
    if (strlen(register_pipe_name) > MAX_NAMED_PIPE_SIZE || strlen(named_pipe) > MAX_NAMED_PIPE_SIZE) {
        fprintf(stderr, "failed: one or more of the arguments is too long\n");
        return -1;
    }
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
        exit(EXIT_FAILURE);
    }    

    char *register_pipe_name = argv[1]; // assign the register pipe name
    char *pipe_name = argv[2];          // assign the pipe name
    char *command = argv[3];            // assign the command

    if (check_args(register_pipe_name, pipe_name) == -1) {
        fprintf(stderr, "failed: invalid arguments\n");
        return -1;
    }

    // unlink pipe_name if it already exists
    if (unlink(pipe_name) < 0 && errno != ENOENT) {
        fprintf(stderr, "failed: could not unlink pipe: %s\n", pipe_name);
        exit(EXIT_FAILURE);
    }

    char* path = "/tmp/";
    char* full_path = malloc(strlen(path) + strlen(pipe_name) + 1);
    memset(full_path, path, strlen(path));
    memset(full_path + strlen(path), pipe_name, strlen(pipe_name));


    if (mkfifo(pipe_name, 0640) < 0) {
        fprintf(stderr, "failed: could not create pipe: %s\n", pipe_name);
        exit(EXIT_FAILURE);
    } else {
    }

    if (argc > 4){                      // this means we're either creating or removing a box
        char *box_name = argv[4];
        if (strlen(box_name) > MAX_BOX_NAME_SIZE) {
            exit(EXIT_FAILURE);
        }
        if (strcmp(command, "create") == 0) {
            register_box_request(register_pipe_name, pipe_name, box_name);
        } else if (strcmp(command, "remove") == 0) {
            remove_box(register_pipe_name, pipe_name, box_name);
        }
    } else {                            // this means we're listing the boxes
        list_boxes_request(register_pipe_name, pipe_name);
    }

    int pipe_fd = open(pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", pipe_name);
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    while (true){
        int select_result = read_pipe_input(pipe_fd, read_fds);
        if (select_result == -1) {
            fprintf(stderr, "failed: could not read from pipe: %s\n", pipe_name);
            exit(EXIT_FAILURE);
        } else if (select_result == 1) {                // was successful
            break;
        }
        // continue waiting for input
    }

    // close the pipe and exit
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", pipe_name);
        exit(EXIT_FAILURE);
    }

    // remove the named pipe
    if (unlink(pipe_name) < 0) {
        fprintf(stderr, "failed: could not remove pipe: %s\n", pipe_name);
        exit(EXIT_FAILURE);
    }

    return 0;
}