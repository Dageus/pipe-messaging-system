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


int end_session(){
    // IMPORTANT
}
// if a box is removed by the manager or a client closes their named pipe
// subtract 1 to the open sessions

box_t* get_box_by_publisher_pipe(char* publisher_pipe_name){
    // find the box that has the publisher pipe
    box_list_t* box_node = box_list;
    while (box_node != NULL) {
        if (strcmp(box_node->box->publisher->named_pipe, publisher_pipe_name) == 0) {
            return box_node->box;
        }
        box_node = box_node->next;
    }
    return NULL;
}

int spread_message(char* message, char* publisher_pipe_name) {
    // send the message to all the subscribers of the box
    // for each subscriber, send the message to the pipe
    box_t* box = get_box_by_publisher_pipe(publisher_pipe_name);
    if (box == NULL){
        return -1;
    }

    // iterate through the subscribers and send message to their pipes
    subscriber_list_t* subscribers = box->subscribers;
    while (subscribers != NULL) {
        int pipe_fd = open(subscribers->subscriber->named_pipe, O_WRONLY);
        if (pipe_fd < 0) {
            return -1;
        }
        ssize_t num_bytes;
        num_bytes = write(pipe_fd, message, strlen(message));
        if (num_bytes < 0) {
            return -1;
        }
        subscribers = subscribers->next;
    }

    return 0;
}

box_t* find_box_by_name(char* box_name) {
    //Find the box in the list
    while (box_list != NULL){
        if (strcmp(box_list->box->box_name, box_name) == 0) {
            return box_list->box;
        }
        box_list = box_list->next;
    }
    return NULL;
}

int create_box(char* box_name){
    // Verify if the box already exists
    if (find_box_by_name(box_name) != NULL)
        return -1;

    // Create the box
    box_t* box = malloc(sizeof(box_t));
    box->box_name = malloc(sizeof(char) * (strlen(box_name) + 1));
    strcpy(box->box_name, box_name);
    box->publisher = NULL;
    box->subscribers = NULL;
    box->num_subscribers = 0;

    // Add the box to the list
    box_list_t* box_node = malloc(sizeof(box_list_t));
    box_node->box = box;
    box_node->next = box_list;
    box_list = box_node;

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


int register_publisher_command(char* client_named_pipe_path, char* box_name) {
    // Find box in the list
    box_t* box = find_box_by_name(box_name);
    if (box == NULL)
        return -1;

    // Verify if the box already has a publisher
    if (box->publisher != NULL)
        return -1;

    // Create the publisher
    publisher_t* publisher = malloc(sizeof(publisher_t));
    strcpy(publisher->named_pipe, client_named_pipe_path);
    strcpy(publisher->box_name, box_name);

    // Add the publisher to the box
    box->publisher = publisher;

    return 0;
}

int register_subscriber_command(char* client_named_pipe_path, char* box_name) {
    // Find box in the list
    box_t* box = find_box_by_name(box_name);
    if (box == NULL)
        return -1;

    // add the subscriber to the box
    subscriber_t* subscriber = malloc(sizeof(subscriber_t));
    strcpy(subscriber->named_pipe, client_named_pipe_path);
    strcpy(subscriber->box_name, box_name);

    if (box->subscribers == NULL) {   // no subscribers yet
        box->subscribers = malloc(sizeof(subscriber_list_t));
        box->subscribers->subscriber = subscriber;
        box->subscribers->next = NULL;
    } else {                                    // there are already subscribers
        subscriber_list_t* subscriber_node = box->subscribers;
        while (subscriber_node->next != NULL) {
            subscriber_node = subscriber_node->next;
        }
        subscriber_list_t* new_subscriber_node = malloc(sizeof(subscriber_list_t));
        new_subscriber_node->subscriber = subscriber;
        new_subscriber_node->next = NULL;
        subscriber_node->next = new_subscriber_node;
    }

    return 0;
}


// completed functions


// message format:
//[ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
char* create_listing_message(box_list_t* box_node) {
    // create the message
    char message[56]; // 1 + 1 + 32 + 8 + 8 + 8 = 56 bytes

    // code
    u_int8_t code = 8;
    memcpy(message, &code, sizeof(code));
    
    // last
    u_int8_t last = 0;
    if (box_node->next == NULL) {
        last = 1;
    }
    memcpy(message, &last, sizeof(last));
    
    // box_name
    memcpy(message, box_node->box->box_name, sizeof(box_node->box->box_name));

    // box_size
    u_int64_t box_size = sizeof(box_node->box);
    memcpy(message, &box_size, sizeof(box_size));

    // n_publishers
    u_int64_t n_publishers = 0;
    if (box_node->box->publisher != NULL) {
        n_publishers = 1;
    }
    memcpy(message, &n_publishers, sizeof(n_publishers));

    // n_subscribers
    u_int64_t n_subscribers = box_node->box->num_subscribers;
    memcpy(message, &n_subscribers, sizeof(n_subscribers));

    return message;
}


//This function is very incomplete and possibly right now it only iterates through the boxes and creates a message
//for each box. It does not send the message to the pipe.
int list_boxes_command(char* manager_pipe_name){
    // list all the boxes in the list
    // return 0 if success, -1 if error

    int pipe_fd = open(manager_pipe_name, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", manager_pipe_name);
        return -1;
    }

    // iterate through the boxes and creat a message for each box
    box_list_t* box_node = box_list;
    while (box_node != NULL) {
        char* message = create_listing_message(box_node);
        if (write(pipe_fd, message, sizeof(message)) < 0) {
            fprintf(stderr, "failed: could not write to pipe: %s\n", manager_pipe_name);
            return -1;
        }
        box_node = box_node->next;
    }

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", manager_pipe_name);
        return -1;
    }

    return 0;
}

// this function is the one that will be used in case the publisher sends a messaggge
int pub_to_box(char *box_name) {
    // check if box exists
    if (!box_in_list(box_name)) {
        return -1;
    }

    return 0;
}

int read_publisher_pipe_input(int pipe_fd, fd_set read_fds, char* publisher_named_pipe){
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

        if (code == 9){
            char message[1024];
            ssize_t num_bytes = read(pipe_fd, message, sizeof(message));
            if (num_bytes < 0) { // error
                return -1;
            }
            // send the message to all the subscribers of the box
            if (spread_message(message, publisher_named_pipe) < 0) {
                return -1;
            }
        }

    }

    return 0;
}

int listen_to_publisher(char* publisher_named_pipe){
    // listen to the publisher, the same way as manager.c and sub.c
    int pipe_fd = open(publisher_named_pipe, O_RDONLY);
    if (pipe_fd < 0) {
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);
    while(true){
        // read the message from the pipe
        // send the message to the subscribers
        if (read_publisher_pipe_input(pipe_fd, read_fds, publisher_named_pipe) < 0) {
            return -1;
        } else{
            // send the message to the subscribers
        }
    }
}

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
            if (register_publisher_command(client_named_pipe_path, box_name) < 0) {
                return -1;
            }

            // listen to the publisher on his pipe in a new thread
            if (listen_to_publisher(client_named_pipe_path) < 0) {
                return -1;
            }

            //pthread_t publisher_thread;
            //if (pthread_create(&publisher_thread, NULL, listen_to_publisher, (void*)client_named_pipe_path) < 0) {
            //    return -1;
            //}
            // ↑↑↑↑↑↑ inutil, copilot é que fez

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
            if (create_box_command(box_name) < 0) {
                return -1;
            }

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

            if (remove_box_command(box_name) < 0) {
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

            // TODO: this function only creats the list of messages doesn't send them anywhere
            if (list_boxes_command(client_named_pipe_path) < 0) {
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
        return -1;
    }
    // create the named pipe
    if (mkfifo(mbroker->register_pipe_name, 0666) < 0) {
        return -1;
    }

    int pipe_fd = open(mbroker->register_pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
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
