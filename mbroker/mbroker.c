#include "logging.h"
#include "structures.h"
#include "messages.h"
#include "operations.h"
#include "state.h"
//#include "producer-consumer.h"
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
#include <signal.h>
#include <pthread.h>

/*
 * a session remains open until either:
 *      - the publisher or subscriber closes the pipe
 *      - a box is removed from the manager 
 */

mbroker_t *mbroker;

box_list_t *box_list = NULL;

// array to keep track of the sessions

// array to keep track of the pipes

// IMPORTANT: 
// - functions to implement yet:

box_t* get_box_by_publisher();

int spread_message(char* message, box_t* box) {
    // send the message to all the subscribers of the box
    // for each subscriber, send the message to the pipe

    // iterate through the subscribers and send message to their pipes
    subscriber_list_t* subscriber = box->subscribers;



    return 0;
}

int remove_box_command(char* box_name) {
    // remove the box from the list
    // free the memory
    // close the pipes
    // return 0 if success, -1 if error

    // find the box
    box_list_t* box_node = box_list;
    box_list_t* prev_box_node = NULL;
    while (box_node != NULL) {
        if (strcmp(box_node->box->box_name, box_name) == 0) {
            break;
        }
        prev_box_node = box_node;
        box_node = box_node->next;
    }
    if (box_node == NULL) {
        return -1;
    }

    // remove the box from the list
    if (prev_box_node == NULL) {
        box_list = box_node->next;
    } else {
        prev_box_node->next = box_node->next;
    }

    // free the memory
    free(box_node->box->box_name);
    free(box_node->box);
    free(box_node);

    return 0;
}

int register_subscriber_command(char* client_named_pipe_path, char* box_name) {
    // find the box
    box_list_t* box_node = box_list;
    while (box_node != NULL) {
        if (strcmp(box_node->box->box_name, box_name) == 0) {
            break;
        }
        box_node = box_node->next;
    }
    if (box_node == NULL) {
        return -1;
    }

    // add the subscriber to the box
    subscriber_t* subscriber = malloc(sizeof(subscriber_t));
    subscriber->client_fd = client_named_pipe_path;
    strcpy(subscriber->box_name, box_name);

    if (box_node->box->subscribers == NULL) {   // no subscribers yet
        box_node->box->subscribers = malloc(sizeof(subscriber_list_t));
        box_node->box->subscribers->subscriber = subscriber;
        box_node->box->subscribers->next = NULL;
    } else {                                    // there are already subscribers
        subscriber_list_t* subscriber_node = box_node->box->subscribers;
        while (subscriber_node->next != NULL) {
            subscriber_node = subscriber_node->next;
        }
        subscriber_node->next = subscriber;
    }

    return 0;
}

int end_session(); 
// if a box is removed by the manager or a client closes their named pipe
// subtract 1 to the open sessions


// completed functions


int process_command(int pipe_fd, u_int8_t code) {


    // read the message from the pipe
    switch (code)
    {
        // register a publisher
        case 1:{
            char client_named_pipe_path[256];
            ssize_t num_bytes;
            num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(client_named_pipe_path));
            if (num_bytes < 0) { // error
                return -1;
            }
            // parse the box name
            char box_name[32];
            num_bytes = read(pipe_fd, box_name, sizeof(box_name));
            if (num_bytes < 0) { // error
                return -1;
            }
            // register the publisher
            /*
            
            TO DO: creat function register_publisher_command
            
            if (register_publisher_command(client_named_pipe_path, box_name) < 0) {
                return -1;
            }
            */
            
            break;
        }
        // register a subscriber
        case 2:{
            // parse the client pipe name
            char client_named_pipe_path[256];
            ssize_t num_bytes;
            num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(client_named_pipe_path));
            if (num_bytes < 0) { // error
                return -1;
            }
            // parse the box name
            char box_name[32];
            num_bytes = read(pipe_fd, box_name, sizeof(box_name));
            if (num_bytes < 0) { // error
                return -1;
            }
            // register the subscriber

            if (register_subscriber_command(client_named_pipe_path, box_name) < 0) {
                return -1;
            }
            
            break;
        }
        // create a box
        case 3:{
            // parse the client pipe name
            char client_named_pipe_path[256];
            ssize_t num_bytes;
            num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(client_named_pipe_path));
            if (num_bytes < 0) { // error
                return -1;
            }
            // parse the box name
            char box_name[32];
            num_bytes = read(pipe_fd, box_name, sizeof(box_name));
            if (num_bytes < 0) { // error
                return -1;
            }
            // create the box
            /*

            TO DO: creat function create_box_command

            if (create_box_command( box_name) < 0) {
                return -1;
            }
            */

            break;
        }
        // remove a box
        case 5:{    
            // parse the client pipe name
            char client_named_pipe_path[256];
            ssize_t num_bytes;
            num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(client_named_pipe_path));
            if (num_bytes < 0) { // error
                return -1;
            }
            // parse the box name
            char box_name[32];
            num_bytes = read(pipe_fd, box_name, sizeof(box_name));
            if (num_bytes < 0) { // error
                return -1;
            }
            // remove the box

            if (remove_box_command( box_name) < 0) {
                return -1;
            }
        
            break;
        }
        // list boxes
        case 7:{
            // parse the client pipe name
            char client_named_pipe_path[256];
            ssize_t num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(client_named_pipe_path));
            if (num_bytes < 0) { // error
                return -1;
            }
            // send the list of boxes to the client
            /*

            TO DO: creat function list_boxes_command

            if (list_boxes_command(client_named_pipe_path) < 0) {
                return -1;
            }
            */
        
            break;
        }
        // publisher sends a message
        case 9:{
            // read the message from the pipe
            char message[1024];
            ssize_t num_bytes = read(pipe_fd, message, sizeof(message));
            if (num_bytes < 0) { // error
                return -1;
            }
            // send the message to all the subscribers of the box
            if (spread_message(message) < 0) {
                return -1;
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

    }

    return 0;
}

void add_box_to_list(char* box_name){
    // find the last box in the list
    while (box_list != NULL) {
        box_list = box_list->next;
    }
    // add the new box to the list
    box_list->next = new_node(box_name);
}

int box_in_list(char* box_name){
    // iterate the box list and find if there is a box with the same name
    while (box_list != NULL) {
        if (strcmp(box_list->box->box_name, box_name) == 0) {
            return 1;
        }
        box_list = box_list->next;
    }
    return 0;
}


int answer_to_pipe(u_int8_t code, char* client_named_pipe_path){
    (void) code; // unused parameter
    (void) client_named_pipe_path; // unused parameter
    // write to pipe to answer to client
    
    return 0;
}

int create_box(char *box_name) {
    // check if box already exists
    if (box_list == NULL) {
        tfs_open(box_name, O_CREAT);
        box_list = new_node(box_name);
    } else {
        // check if box is already in the list
        if (!box_in_list(box_name)) {
            // add box to the list
            add_box_to_list(box_name);
        }
    }
    return 0;
}

int pub_to_box(char *box_name) {
    // check if box exists
    if (!box_in_list(box_name)) {
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
