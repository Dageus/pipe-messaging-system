#include "logging.h"
#include "structures.h"
#include "messages.h"
#include "operations.h"
#include "state.h"
#include "producer-consumer.h"
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

// struct to hold the mbroker state
mbroker_t *mbroker;

// array to keep track of the boxes
box_list_t *box_list = NULL;

// queue to avoid busy waiting
pc_queue_t *pc_queue = NULL;

// array to keep track of the threads
pthread_t *thread_array = NULL;

// mutex to lock the box_list
static pthread_mutex_t box_list_mutex = PTHREAD_MUTEX_INITIALIZER;

// array to keep track of the pipes

box_t* get_box_by_publisher_pipe(char* publisher_pipe_name) {
    fprintf(stderr, "in get_box_by_publisher_pipe\n");
    // lock the mutex
    if (pthread_mutex_lock(&box_list_mutex) == -1) {
        WARN("failed to lock mutex: %s", strerror(errno));
        return NULL;
    } 
    fprintf(stderr, "locked mutex in get_box_by_publisher_pipe\n");
    
    // find the box that has the publisher pipe
    box_list_t* box_node = box_list;
    while (box_node != NULL) {
        if (strcmp(box_node->box->publisher_named_pipe, publisher_pipe_name) == 0) {
            pthread_mutex_unlock(&box_list_mutex); // unlock the mutex
            return box_node->box;
        }
        box_node = box_node->next;
    }
    pthread_mutex_unlock(&box_list_mutex); // unlock the mutex
    return NULL;
}

int box_in_list(char* box_name){
    // lock the mutex
    if (pthread_mutex_lock(&box_list_mutex) == -1) {
        WARN("failed to lock mutex: %s", strerror(errno));
        return -1;
    } 

    box_list_t* box_node = box_list;
    while (box_node != NULL) {
        if (strcmp(box_node->box->box_name, box_name) == 0) {
            pthread_mutex_unlock(&box_list_mutex); // unlock the mutex
            return 1;
        }
        box_node = box_node->next;
    }
    pthread_mutex_unlock(&box_list_mutex); // unlock the mutex
    return 0;
}


char* create_message(u_int8_t code, char* message_to_write){
    char *message = (char*) malloc(sizeof(char) * 1025);
    memcpy(message, &code, sizeof(u_int8_t));
    memcpy(message + 1, message_to_write, strlen(message_to_write));
    memset(message + 1 + strlen(message_to_write), '\0', 1024 - strlen(message_to_write));
    return message;
}


int spread_message(char* message_to_write, char* publisher_pipe_name) {
    // send the message to all the subscribers of the box
    // for each subscriber, send the message to the pipe
    box_t* box = get_box_by_publisher_pipe(publisher_pipe_name);
    fprintf(stderr, "[DEUBG]: got box\n");
    if (box == NULL) {  // if the box does not exist
        return -1;
    }

    // iterate through the subscribers and send message to their pipes
    subscriber_list_t* subscribers = box->subscribers;
    while (subscribers != NULL) {
        int pipe_fd = open(subscribers->subscriber->named_pipe, O_WRONLY);
        if (pipe_fd < 0) {
            fprintf(stderr, "[ERROR]: Could not open pipe %s with error %s\n", subscribers->subscriber->named_pipe, strerror(errno));
            return -1;
        }

        u_int8_t code = OP_CODE_SUBSCRIBER_MESSAGE;
        char* message = create_message(code, message_to_write);
        ssize_t num_bytes;

        num_bytes = write(pipe_fd, message, 1025);
        if (num_bytes < 0) {
            return -1;
        }
        // get the next subscriber
        subscribers = subscribers->next;
    }
    return 0;
}


box_t* find_box_by_name(char* box_name) {
    //Find the box in the list
    box_list_t* dummy_box_list = box_list;
    while (dummy_box_list != NULL){
        if (strcmp(dummy_box_list->box->box_name, box_name) == 0) {
            return dummy_box_list->box;
        }
        dummy_box_list = dummy_box_list->next;
    }
    return NULL;
}

int get_old_messages(box_t* box, char* subscriber_pipe_name) {
    // lock the mutex

    // open the box using the box name and tfs
    int box_fd = tfs_open(box->box_name, TFS_O_APPEND);

    if (box_fd < 0) {
        fprintf(stderr, "[ERROR]: Could not open box %s\n", box->box_name);
        return -1;
    }

    if (tfs_rewind_offset(box_fd) < 0) {
        fprintf(stderr, "[ERROR]: Could not rewind offset for box %s\n", box->box_name);
        return -1;
    }
    
    // read the messages from the box
    message_list_t* messages = box->messages_size;
    char* message = (char*) malloc(sizeof(char) * MAX_MESSAGE_SIZE);
    ssize_t num_bytes = tfs_read(box_fd, message, messages->message_size);

    if (num_bytes < 0) {
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
        fprintf(stderr, "[ERROR]: Could not read message from box %s: %s\n", box->box_name, strerror(errno));
        return -1;
    }
    while (true) {
        // send the message to the subscriber
        int pipe_fd = open(subscriber_pipe_name, O_WRONLY);
        if (pipe_fd < 0) {
            fprintf(stderr, "[ERROR]: Could not open pipe %s with error %s\n", subscriber_pipe_name, strerror(errno));
            return -1;
        }
        u_int8_t code = OP_CODE_SUBSCRIBER_MESSAGE;
        char* message_to_send = create_message(code, message);
        
        num_bytes = write(pipe_fd, message_to_send, 1025);
        if (num_bytes < 0) {
            return -1;
        }

        if (messages->next == NULL) {
            break;
        }

        // get the next message size
        messages = messages->next;

        memset(message, 0, sizeof(char) * MAX_MESSAGE_SIZE);
        
        num_bytes = tfs_read(box_fd, message, messages->message_size);
    }

    return 0;
}

char* create_answer(u_int8_t code, int32_t return_code, char* error_message, unsigned int size_of_answer){
    char *answer = (char*) malloc(sizeof(char) * size_of_answer); 
    memcpy(answer, &code, sizeof(u_int8_t));
    memcpy(answer + sizeof(u_int8_t), &return_code, sizeof(int32_t));
    if (return_code < 0){
        memcpy(answer + sizeof(u_int8_t) + sizeof(int32_t), error_message, strlen(error_message));
        memset(answer + sizeof(u_int8_t) + sizeof(int32_t) + strlen(error_message), '\0', sizeof(char) * (1024 - strlen(error_message)));
        INFO("Answer created: %s", answer);
        return answer;
    }
    memset(answer + sizeof(int32_t) + sizeof(u_int8_t), '\0', sizeof(char) * 1024);
    return answer;
}

int32_t create_box_command(char* box_name){
    // lock the mutex
    if (pthread_mutex_lock(&box_list_mutex) == -1) {
        WARN("failed to lock mutex: %s", strerror(errno));
        return -1;
    }
    fprintf(stderr, "[DEBUG]: locked mutex\n");
    // Verify if the box already exists
    if (find_box_by_name(box_name) != NULL){
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
        return -1;
    }

    // Create the box
    int box_fd = tfs_open(box_name, TFS_O_CREAT);
    if (box_fd < 0){
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
        return -1;
    }

    fprintf(stderr, "[DEBUG]: box created\n");
    // Add the box to the list
    box_t* box = (box_t*) malloc(sizeof(box_t));
    box->box_name = (char*) malloc(sizeof(char) * 32);
    strcpy(box->box_name, box_name);
    box->publisher_named_pipe = NULL;
    box->subscribers = NULL;
    box->num_subscribers = 0;
    box->messages_size = NULL;
    if (box_list == NULL){
        fprintf(stdout, "FIRST BOX\n");
        box_list = (box_list_t*) malloc(sizeof(box_list_t));
        box_list->box = box;
        box_list->next = NULL;
    } else {
        fprintf(stdout, "FIRST BOX NAME IN LIST: %s\n", box_list->box->box_name);
        box_list_t* box_node = box_list;
        while (box_node->next != NULL){
            box_node = box_node->next;
        }
        box_list_t* new_box_node = (box_list_t*) malloc(sizeof(box_list_t));
        new_box_node->box = box;
        new_box_node->next = NULL;
        box_node->next = new_box_node;
    }

    // unlock the mutex
    if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
        WARN("failed to unlock mutex: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int remove_box_command(char* box_name) {
    // lock the mutex
    if (pthread_mutex_lock(&box_list_mutex) == -1) {
        WARN("failed to lock mutex: %s", strerror(errno));
        return -1;
    }
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
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
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

    // unlock the mutex
    if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
        WARN("failed to unlock mutex: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int register_publisher_command(char* client_named_pipe_path, char* box_name) {
    // lock the mutex
    if (pthread_mutex_lock(&box_list_mutex) == -1) {
        WARN("failed to lock mutex: %s", strerror(errno));
        return -1;
    }
    // Find box in the list
    box_t* box = find_box_by_name(box_name);
    if (box == NULL){
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
        return -1;
    }

    // Verify if the box already has a publisher
    if (box->publisher_named_pipe != NULL){
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
        return -1;
    }

    // Create the publisher
    publisher_t* publisher = malloc(sizeof(publisher_t));
    strcpy(publisher->named_pipe, client_named_pipe_path);
    publisher->box_name = box_name;

    // Add the publisher to the box
    box->publisher_named_pipe = client_named_pipe_path;

    // unlock the mutex
    if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
        WARN("failed to unlock mutex: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int remove_publisher_command(char* client_named_pipe_path, char* box_name) {
    // lock the mutex
    if (pthread_mutex_lock(&box_list_mutex) == -1) {
        WARN("failed to lock mutex: %s", strerror(errno));
        return -1;
    }
    // Find box in the list
    box_t* box = find_box_by_name(box_name);
    if (box == NULL){
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
        return -1;
    }
    // Verify if the publisher is the one we want to remove
    if (strcmp(box->publisher_named_pipe, client_named_pipe_path) != 0){
        if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
            WARN("failed to unlock mutex: %s", strerror(errno));
            return -1;
        }
        return -1;
    }

    // Remove the publisher
    box->publisher_named_pipe = NULL;

    // unlock the mutex
    if (pthread_mutex_unlock(&box_list_mutex) == -1){ // unlock the mutex
        WARN("failed to unlock mutex: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int register_subscriber_command(char* client_named_pipe_path, char* box_name) {
    // lock the box
    if (pthread_mutex_lock(&box_list_mutex) == -1){
        return -1;
    }
    // Find box in the list
    box_t* box = find_box_by_name(box_name);
    if (box == NULL){
        if (pthread_mutex_unlock(&box_list_mutex) == -1){
            return -1;
        }
        return -1;
    }
    
    fprintf(stdout, "BOX NAME: %s\n", box->box_name);

    // add the subscriber to the box
    subscriber_t* subscriber = malloc(sizeof(subscriber_t));
    strcpy(subscriber->named_pipe, client_named_pipe_path);
    strcpy(subscriber->box_name, box_name);

    if (box->subscribers == NULL) {   // no subscribers yet
        box->subscribers = malloc(sizeof(subscriber_list_t));
        box->subscribers->subscriber = subscriber;
        box->subscribers->next = NULL;
    } else {                                    // there are already subscribers
        subscriber_list_t* new_subscriber_node = malloc(sizeof(subscriber_list_t));
        new_subscriber_node->subscriber = subscriber;
        new_subscriber_node->next = box->subscribers;
    }

    fprintf(stdout, "SUBSCRIBER ADDED\n");

    if (get_old_messages(box, client_named_pipe_path) < 0) {
        if (pthread_mutex_unlock(&box_list_mutex) == -1){
            return -1;
        }
        return -1;
    }

    // unlock the box
    if (pthread_mutex_unlock(&box_list_mutex) == -1){
        return -1;
    }
    return 0;
}

char* create_listing_message(box_list_t* box_node) {
    // create the message
    char *message = (char*) malloc(sizeof(char) * 58); // 1 + 1 + 32 + 8 + 8 + 8 = 56 bytes

    // code
    u_int8_t code = OP_CODE_ANSWER_TO_LIST;
    memcpy(message, &code, sizeof(u_int8_t));
    
    // last
    u_int8_t last = 0;
    if (box_node->next == NULL) {
        last = 1;
    }
    memcpy(message + 1, &last, sizeof(u_int8_t));
    
    // box_name
    char box_name[32];
    strcpy(box_name, box_node->box->box_name);
    memcpy(message + 1 + 1, box_name, strlen(box_name));
    memset(message + 1 + 1 + strlen(box_name), '\0', 32 - strlen(box_name));
    fprintf(stderr, "box_name: %s\n", box_name);

    // box_size
    u_int64_t box_size = sizeof(box_node->box);
    memcpy(message + 1 + 1 + 32, &box_size, sizeof(u_int64_t));

    // n_publishers
    u_int64_t n_publishers = 0;
    if (box_node->box->publisher_named_pipe != NULL) {
        n_publishers = 1;
    }
    memcpy(message + 1 + 1 + 32 + 8, &n_publishers, sizeof(u_int64_t));
    // n_subscribers
    u_int64_t n_subscribers = 0;
    memcpy(message + 1 + 1 + 32 + 8 + 8, &n_subscribers, sizeof(u_int64_t));

    return message;
}

char* create_listing_error_message(){
    size_t message_size = sizeof(u_int8_t) + sizeof(u_int8_t) + (sizeof(char) * 32) + sizeof(u_int64_t) + sizeof(u_int64_t) + sizeof(u_int64_t) + sizeof(u_int64_t);
    char *message = (char*) malloc(sizeof(char) * message_size); // 1 + 1 + 32 + 8 + 8 + 8 = 56 bytes

    // code
    u_int8_t code = OP_CODE_ANSWER_TO_LIST;
    memcpy(message, &code, sizeof(u_int8_t));
    
    // last
    u_int8_t last = 1;
    memcpy(message + sizeof(u_int8_t), &last, sizeof(u_int8_t));
    
    // box_name
    memset(message + sizeof(u_int8_t) + sizeof(u_int8_t), '\0', 32);

    // box_size
    u_int64_t box_size = 0;
    memcpy(message + sizeof(u_int8_t) + sizeof(u_int8_t) + 32, &box_size, sizeof(u_int64_t));

    // n_publishers
    u_int64_t n_publishers = 0;
    memcpy(message + sizeof(u_int8_t) + sizeof(u_int8_t) + 32 + sizeof(u_int64_t), &n_publishers, sizeof(u_int64_t));

    // n_subscribers
    u_int64_t n_subscribers = 0;
    memcpy(message + sizeof(u_int8_t) + sizeof(u_int8_t) + 32 + sizeof(u_int64_t) + sizeof(u_int64_t), &n_subscribers, sizeof(u_int64_t));

    return message;    
}

int list_boxes_command(char* manager_pipe_name){
    // list all the boxes in the list
    // return 0 if success, -1 if error

    int pipe_fd = open(manager_pipe_name, O_WRONLY);
    if (pipe_fd < 0) {
        fprintf(stderr, "failed: could not open pipe: %s\n", manager_pipe_name);
        return -1;
    }

    // iterate through the boxes and creat a message for each box
    // lock the box list
    if (pthread_mutex_lock(&box_list_mutex) == -1){
        return -1;
    }
    box_list_t* box_node = box_list;
    if (box_node == NULL) {
        char* message = create_listing_error_message();
        if (write(pipe_fd, message, 1029) < 0) {
            fprintf(stderr, "failed: could not write to pipe: %s\n", manager_pipe_name);
            if (pthread_mutex_unlock(&box_list_mutex) == -1){
                return -1;
            }
            return -1;
        }
        if (pthread_mutex_unlock(&box_list_mutex) == -1){
            return -1;
        }
        fprintf(stderr, "success: no boxes to list\n");
        return 0;
    }

    char* message = create_listing_message(box_node);
    if (write(pipe_fd, message, sizeof(message)) < 0) {
        fprintf(stderr, "failed: could not write to pipe: %s\n", manager_pipe_name);
        if (pthread_mutex_unlock(&box_list_mutex) == -1){
            return -1;
        }
        return -1;
    }

    while (box_node->next != NULL) {
        box_node = box_node->next;
        fprintf(stderr, "success: found next box: %s\n", box_node->box->box_name);
        message = create_listing_message(box_node);
        if (write(pipe_fd, message, sizeof(message)) < 0) {
            fprintf(stderr, "failed: could not write to pipe: %s\n", manager_pipe_name);
            if (pthread_mutex_unlock(&box_list_mutex) == -1){
                return -1;
            }
            return -1;
        }
    }

    fprintf(stderr, "success: wrote all boxes to pipe: %s\n", manager_pipe_name);

    // close the pipe
    if (close(pipe_fd) == -1) {
        fprintf(stderr, "failed: could not close pipe: %s\n", manager_pipe_name);
        if (pthread_mutex_unlock(&box_list_mutex) == -1){
            return -1;
        }
        return -1;
    }

    // unlock the box list
    if (pthread_mutex_unlock(&box_list_mutex) == -1){
        return -1;
    }
    return 0;
}

int write_to_box(char* box_name, char* message, size_t message_size){
    int box_fd = tfs_open(box_name, TFS_O_APPEND);

    if (box_fd < 0) {
        return -1;
    }

    // write the message to the box
    ssize_t num_bytes = tfs_write(box_fd, message, message_size);
    if (num_bytes < 0) {
        return -1;
    }

    // close the box
    if (tfs_close(box_fd) < 0) {
        return -1;
    }

    // lock the box list
    if (pthread_mutex_lock(&box_list_mutex) == -1){
        return -1;
    }
    box_t *box = find_box_by_name(box_name);
    if (box == NULL) {
        fprintf(stderr, "failed: could not find box: %s\n", box_name);
        if (pthread_mutex_unlock(&box_list_mutex) == -1){
            return -1;
        }
        return -1;
    }
    
    if (box->messages_size == NULL) {
        box->messages_size = malloc(sizeof(message_list_t));
        box->messages_size->message_size = message_size;
        box->messages_size->next = NULL;
    } else {
        // get last message_size
        message_list_t* message_size_node = box->messages_size;
        while (message_size_node->next != NULL) {
            message_size_node = message_size_node->next;
        }

        // add new message_size
        message_list_t* new_message_node = malloc(sizeof(message_list_t));
        new_message_node->message_size = message_size;
        new_message_node->next = NULL;

        message_size_node->next = new_message_node;
    }
    
    // unlock the box list
    if (pthread_mutex_unlock(&box_list_mutex) == -1){
        return -1;
    }
    return 0;
}

int read_publisher_pipe_input(int pipe_fd, char* publisher_named_pipe, char* box_name) {

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    int sel = select(pipe_fd + 1, &read_fds, NULL, NULL, NULL);
    if (sel < 0) {
        return -1;
    } 
    // data is available on the named pipe
    // read the data from the pipe

    fprintf(stderr, "success: data is available on the named pipe\n");

    u_int8_t code;
    ssize_t num_bytes;

    num_bytes = read(pipe_fd, &code, sizeof(code));
    if (num_bytes == 0) {
        // num_bytes == 0 indicates EOF
        fprintf(stderr, "[INFO]: read EOF from pipe: %s\n", publisher_named_pipe);
        return 1;
    } else if (num_bytes == -1) {
        // num_bytes == -1 indicates error
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (code == 9){
        char message[1024];
        num_bytes = read(pipe_fd, message, sizeof(message));
        if (num_bytes < 0) { // error
            return -1;
        }

        fprintf(stderr, "success: read message from pipe: %s\n", publisher_named_pipe);

        // write to the box
        if (write_to_box(box_name, message, strlen(message)) < 0) {
            return -1;
        }

        fprintf(stderr, "success: wrote message to box: %s\n", box_name);

        // send the message to all the subscribers of the box
        if (spread_message(message, publisher_named_pipe) < 0) {
            return -1;
        }
    }

    pthread_mutex_unlock(&box_list_mutex);  // unlock the mutex
    return 0;
}

int listen_to_publisher(char* publisher_named_pipe, char* box_name){
    // find the publisher in the box list
    int pipe_fd = open(publisher_named_pipe, O_RDONLY);
    if (pipe_fd < 0) {
        return -1;
    }

    while(true){
        int ret = read_publisher_pipe_input(pipe_fd, publisher_named_pipe, box_name);
        if (ret < 0) {
            return -1;
        } else if (ret == 1){
            fprintf(stderr, "publisher disconnected: %s\n", publisher_named_pipe);
            break;
        }
    }
    return 0;
}

int register_publisher(int pipe_fd){
    // Verify that the pipe is open
    if (pipe_fd < 0) {
        return -1;
    }

    // parse the client named pipe path
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
    int ret = listen_to_publisher(client_named_pipe_path, box_name);
    if (ret < 0) {
        return -1;
    } else if (ret == 0){
        // the publisher has closed the pipe
        // remove the publisher from the box list
        if (remove_publisher_command(client_named_pipe_path, box_name) < 0) {
            return -1;
        }
    }

    return 0;
}

int register_subscriber(int pipe_fd){
    // verify that the pipe is open
    if (pipe_fd < 0) {
        return -1;
    }

    // parse the client named pipe path
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
    
    return 0;
}

int register_box(int pipe_fd){
    // Verify that the pipe is open
    if (pipe_fd < 0) {
        return -1;
    }

    // parse the client named pipe path
    char client_named_pipe_path[256];
    ssize_t num_bytes;
    num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(char)*256);
    if (num_bytes < 0) { // error
        return -1;
    }

    // parse the box name
    char box_name[32];
    num_bytes = read(pipe_fd, box_name, sizeof(char)*32);
    if (num_bytes < 0) { // error
        return -1;
    }

    // create the box
    int32_t return_code = create_box_command(box_name);
    u_int8_t op_code = OP_CODE_ANSWER_TO_CREATION;
    char *error_message = "failed: could not register box";
    char *answer = create_answer(op_code, return_code, error_message, ANSWER_MESSAGE_SIZE);
    fprintf(stderr, "answer: %s\n", answer);

    // open client pipe
    int client_pipe_fd = open(client_named_pipe_path, O_WRONLY);
    if (client_pipe_fd < 0) {
        return -1;
    }

    // send the answer to the client
    if (write(client_pipe_fd, answer, sizeof(answer)) < 0) {
        return -1;
    }

    // close the client pipe
    if (close(client_pipe_fd) < 0) {
        return -1;
    }

    return 0;
}

int remove_box(int pipe_fd){
    // verify that the pipe is open
    if (pipe_fd < 0) {
        return -1;
    }

    // parse the client named pipe path
    char client_named_pipe_path[256];
    ssize_t num_bytes;
    num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(char)*256);
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
    int32_t return_code = remove_box_command(box_name);
    u_int8_t op_code = OP_CODE_ANSWER_TO_REMOVAL;
    char *answer = create_answer(op_code, return_code, "failed: could not remove box", ANSWER_MESSAGE_SIZE);
    if (return_code < 0) {
        return -1;
    }

    // open client pipe
    int client_pipe_fd = open(client_named_pipe_path, O_WRONLY);
    if (client_pipe_fd < 0) {
        return -1;
    }

    // send the answer to the client
    if (write(client_pipe_fd, answer, sizeof(answer)) < 0) {
        return -1;
    }

    // close the client pipe
    if (close(client_pipe_fd) < 0) {
        return -1;
    }
    
    return 0;

}

int list_boxes(int pipe_fd){
    // Verify that the pipe is open
    if (pipe_fd < 0) {
        return -1;
    }
    
    // parse the client pipe name
    char client_named_pipe_path[256];
    ssize_t num_bytes;
    num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(char)*256);
    if (num_bytes < 0) { // error
        return -1;
    }

    // send the list of boxes to the client
    if (list_boxes_command(client_named_pipe_path) < 0) {
        return -1;
    }

    return 0;
}

int read_pipe_input(int pipe_fd){
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    int sel = select(pipe_fd + 1, &read_fds, NULL, NULL, NULL);
    if (sel < 0) {
        return -1;
    }

    // data is available on the named pipe
    // read the data from the pipe


    // threads will be enqueue in this function

    u_int8_t code;
    ssize_t num_bytes = read(pipe_fd, &code, sizeof(code));
    if (num_bytes == 0) {
        // num_bytes == 0 indicates EOF
        return 0;
    } else if (num_bytes == -1) {
        // num_bytes == -1 indicates error
        exit(EXIT_FAILURE);
    }

    session_t* session = (session_t *)malloc(sizeof(session_t));
    session->pipe_fd = pipe_fd;
    session->op_code = code;

    //pcq_enqueue(pc_queue, &session);
    
    switch (code) {
        // register a publisher
        case OP_CODE_REGISTER_PUBLISHER:{
            fprintf(stderr, "read_pipe_input: OP_CODE_REGISTER_PUBLISHER received\n");
            if (register_publisher(pipe_fd) < 0){
                return -1;
            }

            break;
        }
        // register a subscriber
        case OP_CODE_REGISTER_SUBSCRIBER:{
            fprintf(stderr, "read_pipe_input: OP_CODE_REGISTER_SUBSCRIBER received\n");
            if (register_subscriber(pipe_fd) < 0){
                return -1;
            }
            
            break;
        }
        // create a box
        case OP_CODE_REGISTER_BOX:{
            fprintf(stderr, "read_pipe_input: OP_CODE_REGISTER_BOX received\n");
            if (register_box(pipe_fd) < 0){
                return -1;
            }
            
            break;
        }
        // remove a box
        case OP_CODE_REMOVE_BOX:{   
            fprintf(stderr, "read_pipe_input: OP_CODE_REMOVE_BOX received\n"); 
            if (remove_box(pipe_fd) < 0){
                return -1;
            }
            
            break;
        }
        // list boxes
        case OP_CODE_BOX_LIST:{
            fprintf(stderr, "read_pipe_input: OP_CODE_BOX_LIST received\n");
            if (list_boxes(pipe_fd) < 0){
                return -1;
            }
            
            break;
        }
        default:{                
            fprintf(stderr, "[ERR]: Invalid command code: %d\n", code);
            return -1;
        }
    }

    return 0;
}

void *session_thread() {
    fprintf(stderr, "session_thread: started\n");
    while (true) {
        session_t* data = (session_t *)pcq_dequeue(pc_queue);
        if (data == NULL) {
            fprintf(stderr, "failed: could not dequeue from pcq\n");
            exit(EXIT_FAILURE);
        }

        /*
        switch (data->op_code) {
            // register a publisher
            case OP_CODE_REGISTER_PUBLISHER:{
                if (register_publisher(data->pipe_fd) < 0){
                    return -1;
                }

                break;
            }
            // register a subscriber
            case OP_CODE_REGISTER_SUBSCRIBER:{
                if (register_subscriber(data->pipe_fd) < 0){
                    return -1;
                }
                
                break;
            }
            // create a box
            case OP_CODE_REGISTER_BOX:{
                if (register_box(data->pipe_fd) < 0){
                    return -1;
                }
                
                break;
            }
            // remove a box
            case OP_CODE_REMOVE_BOX:{    
                if (remove_box(data->pipe_fd) < 0){
                    return -1;
                }
                
                break;
            }
            // list boxes
            case OP_CODE_BOX_LIST:{
                if (list_boxes(data->pipe_fd) < 0){
                    return -1;
                }
                
                break;
            }
            default:{                
                fprintf(stderr, "[ERR]: Invalid command code: %d\n", data->op_code);
                return -1;
            }
        }
        */
    }
    return NULL;
}

int init_mbroker(mbroker_t *mbroker_config) {
    // initialize the tfs
    if (tfs_init(NULL) < 0) {
        fprintf(stderr, "failed: could not initialize tfs\n");
        return -1;
    }

    // initialize the pcq
    pc_queue = malloc(sizeof(pc_queue_t));

    if (pcq_create(pc_queue, mbroker_config->max_sessions) < 0) {
        fprintf(stderr, "failed: could not initialize pcq\n");
        return -1;
    }

    // initialize the session threads
    thread_array = malloc(sizeof(pthread_t) * mbroker_config->max_sessions);   // array of threads
    for (int i = 0; i < mbroker_config->max_sessions; i++) {
        //if (pthread_create(&thread_array[i], NULL, session_thread, NULL) < 0) {
        //    fprintf(stderr, "failed: could not create thread\n");
        //    exit(EXIT_FAILURE);
        //}
    }

    // unlink register_pipe_name if it already exists
    if (unlink(mbroker_config->register_pipe_name) < 0 && errno != ENOENT) {
        return -1;
    }

    // create the named pipe
    if (mkfifo(mbroker_config->register_pipe_name, 0640) < 0) {
        fprintf(stderr, "failed: could not create named pipe\n");
        return -1;
    }

    return 0;
}

int close_mbroker(){
    // close the tfs
    if (tfs_destroy() < 0) {
        fprintf(stderr, "failed: could not destroy tfs\n");
        return -1;
    }

    // close the pcq
    if (pcq_destroy(pc_queue) < 0){
        fprintf(stderr, "failed: could not destroy pcq\n");
        return -1;  
    }

    // join the session threads
    for (int i = 0; i < mbroker->max_sessions; i++) {
        pthread_join(thread_array[i], NULL);
    }

    free(mbroker);

    return 0;
}

int check_args(char* register_pipe_name, size_t max_sessions){
    if (register_pipe_name == NULL || max_sessions == 0 || strlen(register_pipe_name) > MAX_NAMED_PIPE_SIZE) {
        fprintf(stderr, "failed: invalid arguments\n");
        return -1;
    }
    return 0;
}

/*
 * format:
 *  - mbroker <register_pipe_name> <max_sessions>
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    mbroker = (mbroker_t*) malloc(sizeof(mbroker_t));

    mbroker->register_pipe_name = argv[1];              // register_pipe_name is the name of the pipe to which the manager wants to connect to
    mbroker->max_sessions = (size_t) atoi(argv[2]);     // max_sessions is the maximum number of sessions that can be open at the same time
    
    if (check_args(mbroker->register_pipe_name, mbroker->max_sessions) < 0) {
        return -1;
    }
    
    // initialize the mbroker
    if (init_mbroker(mbroker) < 0) {
        fprintf(stderr, "failed: could not initialize mbroker\n");
        return -1;
    }

    int pipe_fd = open(mbroker->register_pipe_name, O_RDONLY);
    if (pipe_fd < 0) {
        return -1;
    }

    while (true) {
        if (read_pipe_input(pipe_fd) < 0) {
            fprintf(stderr, "failed: could not read pipe input\n");
            return -1;
        }
    }

    // close the named pipe
    if (close(pipe_fd) < 0) {
        fprintf(stderr, "failed: could not close named pipe\n");
        return -1;
    }

    // unlink the named pipe
    if (unlink(mbroker->register_pipe_name) < 0) {
        fprintf(stderr, "failed: could not unlink named pipe\n");
        return -1;
    }

    // close the mbroker
    if (close_mbroker() < 0) {
        fprintf(stderr, "failed: could not close mbroker\n");
        return -1;
    }

    return 0;
}