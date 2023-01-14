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

mbroker_t *mbroker;

box_list_t *box_list = NULL;

pc_queue_t *pc_queue = NULL;

pthread_t *thread_array = NULL;

pthread_mutex_t mutex;  // define the mutex

// array to keep track of the sessions

// array to keep track of the pipes

// if a box is removed by the manager or a client closes their named pipe
// subtract 1 to the open sessions

box_t* get_box_by_publisher_pipe(char* publisher_pipe_name) {
    pthread_mutex_lock(&mutex); // lock the mutex
    // find the box that has the publisher pipe
    box_list_t* box_node = box_list;
    while (box_node != NULL) {
        if (strcmp(box_node->box->publisher_named_pipe, publisher_pipe_name) == 0) {
            pthread_mutex_unlock(&mutex); // unlock the mutex
            return box_node->box;
        }
        box_node = box_node->next;
    }
    pthread_mutex_unlock(&mutex); // unlock the mutex
    return NULL;
}

int box_in_list(char* box_name){
    pthread_mutex_lock(&mutex); // lock the mutex
    box_list_t* box_node = box_list;
    while (box_node != NULL) {
        if (strcmp(box_node->box->box_name, box_name) == 0) {
            pthread_mutex_unlock(&mutex); // unlock the mutex
            return 1;
        }
        box_node = box_node->next;
    }
    pthread_mutex_unlock(&mutex); // unlock the mutex
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
    if (box == NULL) {  // if the box does not exist
        return -1;
    }

    // iterate through the subscribers and send message to their pipes
    subscriber_list_t* subscribers = box->subscribers;
    while (subscribers != NULL) {
        fprintf(stderr, "[INFO]: Sending message to %s...\n", subscribers->subscriber->named_pipe);
        int pipe_fd = open(subscribers->subscriber->named_pipe, O_WRONLY);
        if (pipe_fd < 0) {
            fprintf(stderr, "[ERROR]: Could not open pipe %s with error %s\n", subscribers->subscriber->named_pipe, strerror(errno));
            return -1;
        }
        u_int8_t code = 10;
        char* message = create_message(code, message_to_write);
        ssize_t num_bytes;
        num_bytes = write(pipe_fd, message, 1025);
        if (num_bytes < 0) {
            return -1;
        }
        fprintf(stderr, "[INFO]: Message sent to %s\n", subscribers->subscriber->named_pipe);
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

char* create_answer(u_int8_t code, int32_t return_code, char* error_message, unsigned int size_of_answer){
    char *answer = (char*) malloc(sizeof(char) * size_of_answer); 
    memcpy(answer, &code, sizeof(u_int8_t));
    memcpy(answer + 1, &return_code, sizeof(int32_t));
    if (return_code < 0){
        memcpy(answer + 1 + 4, error_message, strlen(error_message));
        memset(answer + 1 + 4 + strlen(error_message), '\0', sizeof(char) * (1024 - strlen(error_message)));
        return answer;
    }
    memset(answer + 4 + 1, '\0', sizeof(char) * 1024);
    return answer;
}


int create_box_command(char* box_name){
    // Verify if the box already exists
    if (find_box_by_name(box_name) != NULL)
        return -1;

    // Create the box
    int box_fd = tfs_open(box_name, TFS_O_CREAT);
    if (box_fd < 0){
        return -1;
    }

    // Add the box to the list
    box_list_t* box_node = new_node(box_name);
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
    if (box->publisher_named_pipe != NULL)
        return -1;

    // Create the publisher
    publisher_t* publisher = malloc(sizeof(publisher_t));
    strcpy(publisher->named_pipe, client_named_pipe_path);
    publisher->box_name = box_name;

    // Add the publisher to the box
    box->publisher_named_pipe = client_named_pipe_path;

    return 0;
}

int register_subscriber_command(char* client_named_pipe_path, char* box_name) {
    // Find box in the list
    box_t* box = find_box_by_name(box_name);
    if (box == NULL)
        return -1;

    fprintf(stderr, "[INFO]: Registering subscriber %s to box %s...\n", client_named_pipe_path, box_name);

    // add the subscriber to the box
    subscriber_t* subscriber = malloc(sizeof(subscriber_t));
    strcpy(subscriber->named_pipe, client_named_pipe_path);
    strcpy(subscriber->box_name, box_name);

    fprintf(stderr, "[INFO]: Subscriber %s registered to box %s\n", client_named_pipe_path, box_name);

    if (box->subscribers == NULL) {   // no subscribers yet
        box->subscribers = malloc(sizeof(subscriber_list_t));
        box->subscribers->subscriber = subscriber;
        box->subscribers->next = NULL;
    } else {                                    // there are already subscribers
        subscriber_list_t* new_subscriber_node = malloc(sizeof(subscriber_list_t));
        new_subscriber_node->subscriber = subscriber;
        new_subscriber_node->next = box->subscribers;
    }

    fprintf(stderr, "[INFO]: Subscriber %s registered to box successfully\n", client_named_pipe_path);

    return 0;
}


// completed functions


// message format:
//[ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
char* create_listing_message(box_list_t* box_node) {
    // create the message
    char *message = (char*) malloc(sizeof(char) * 56); // 1 + 1 + 32 + 8 + 8 + 8 = 56 bytes
    long unsigned int current_index = 0;

    // code
    u_int8_t code = 8;
    memcpy(message, &code, sizeof(code));
    current_index += sizeof(code);
    
    // last
    u_int8_t last = 0;
    if (box_node->next == NULL) {
        last = 1;
    }
    memcpy(message + current_index, &last, sizeof(last));
    current_index += sizeof(last);
    
    // box_name
    char box_name[32];
    strcpy(box_name, box_node->box->box_name);
    memcpy(message + current_index, box_name, sizeof(box_name));
    current_index += sizeof(box_node->box->box_name);

    // box_size
    u_int64_t box_size = sizeof(box_node->box);
    memcpy(message + current_index, &box_size, sizeof(box_size));
    current_index += sizeof(box_size);

    // n_publishers
    u_int64_t n_publishers = 0;
    if (box_node->box->publisher_named_pipe != NULL) {
        n_publishers = 1;
    }
    memcpy(message + current_index, &n_publishers, sizeof(n_publishers));
    current_index += sizeof(n_publishers);

    // n_subscribers
    u_int64_t n_subscribers = box_node->box->num_subscribers;
    memcpy(message + current_index, &n_subscribers, sizeof(n_subscribers));

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

int write_to_box(char* box_name, char* message, size_t message_size){
    int box_fd = tfs_open(box_name, TFS_O_APPEND);

    if (box_fd < 0) {
        return -1;
    }

    fprintf(stderr, "[INFO]: box_fd: %d\n", box_fd);

    // write the message to the box
    ssize_t num_bytes = tfs_write(box_fd, message, message_size);
    if (num_bytes < 0) {
        return -1;
    }

    fprintf(stderr, "[INFO]: num_bytes: %ld\n", num_bytes);

    // close the box
    if (tfs_close(box_fd) < 0) {
        return -1;
    }

    fprintf(stderr, "[INFO]: wrote to box successfully\n");
    
    return 0;
}

int read_from_box(char* box_name, char* message, size_t message_size){
    int box_fd = tfs_open(box_name, TFS_O_APPEND);
    if (box_fd < 0) {
        return -1;
    }
    
    //read the message from the box
    ssize_t num_bytes = tfs_read(box_fd, message, message_size);
    if (num_bytes < 0) {
        return -1;
    }

    // close the box
    if (tfs_close(box_fd) < 0) {
        return -1;
    }
    
    return 0;
}




int read_publisher_pipe_input(int pipe_fd, fd_set read_fds, char* publisher_named_pipe, char* box_name) {
    int sel = select(pipe_fd + 1, &read_fds, NULL, NULL, NULL);
    if (sel < 0) {
        return -1;
    } else {
        // data is available on the named pipe
        // read the data from the pipe
        pthread_mutex_lock(&mutex);   // lock the mutex
        u_int8_t code;
        ssize_t num_bytes;
        num_bytes = read(pipe_fd, &code, sizeof(code));
        if (num_bytes == 0) {
            // num_bytes == 0 indicates EOF
            pthread_mutex_unlock(&mutex);  // unlock the mutex
            return 0;
        } else if (num_bytes == -1) {
            // num_bytes == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            pthread_mutex_unlock(&mutex);  // unlock the mutex
            exit(EXIT_FAILURE);
        }

        fprintf(stderr, "code: %d\n", code);

        if (code == 9){
            fprintf(stderr, "[INFO]: received message from publisher\n");
            char message[1024];
            num_bytes = read(pipe_fd, message, sizeof(message));
            if (num_bytes < 0) { // error
                pthread_mutex_unlock(&mutex);  // unlock the mutex
                return -1;
            }

            fprintf(stderr, "[INFO]: message: %s\n", message);

            // write to the box
            if (write_to_box(box_name, message, sizeof(message)) < 0) {
                pthread_mutex_unlock(&mutex);  // unlock the mutex
                return -1;
            }

            // send the message to all the subscribers of the box
            if (spread_message(message, publisher_named_pipe) < 0) {
                pthread_mutex_unlock(&mutex);  // unlock the mutex
                return -1;
            }
        }
        pthread_mutex_unlock(&mutex);  // unlock the mutex
    }
    return 0;
}


int listen_to_publisher(char* publisher_named_pipe, char* box_name){
    // find the publisher in the box list
    int pipe_fd = open(publisher_named_pipe, O_RDONLY);
    if (pipe_fd < 0) {
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);
    while(true){
        pthread_mutex_lock(&mutex);   // lock the mutex
        // read the message from the pipe
        // send the message to the subscribers
        if (read_publisher_pipe_input(pipe_fd, read_fds, publisher_named_pipe, box_name) < 0) {
            pthread_mutex_unlock(&mutex);  // unlock the mutex
            return -1;
        } else {
            pthread_mutex_unlock(&mutex);  // unlock the mutex
            break;
        }
    }
    return 0;
}

int process_command(int pipe_fd, u_int8_t code) {
    // read the message from the pipe
    switch (code)
    {
        // register a publisher
        case OP_CODE_REGISTER_PUBLISHER:{
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
            if (listen_to_publisher(client_named_pipe_path, box_name) < 0) {
                return -1;
            }

            break;
        }
        // register a subscriber
        case OP_CODE_REGISTER_SUBSCRIBER:{
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

            break;
        }
        // create a box
        case OP_CODE_REGISTER_BOX:{
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
            u_int8_t op_code = 4;
            char *answer = create_answer(op_code, return_code, "henlo", ANSWER_MESSAGE_SIZE);
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

            break;
        }
        // remove a box
        case OP_CODE_BOX_REMOVAL:{    
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

            // remove the box
            int32_t return_code = remove_box_command(box_name);
            u_int8_t op_code = 6;
            char *answer = create_answer(op_code, return_code, "henloo 2", ANSWER_MESSAGE_SIZE);
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
            
            break;
        }
        // list boxes
        case OP_CODE_BOX_LIST:{
            // parse the client pipe name
            char client_named_pipe_path[256];
            ssize_t num_bytes = read(pipe_fd, client_named_pipe_path, sizeof(client_named_pipe_path));
            if (num_bytes < 0) { // error
                return -1;
            }
            // send the list of boxes to the client

            if (list_boxes_command(client_named_pipe_path) < 0) {
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

/*
void *session_thread(void *arg) {
    void (thread_func) (void *);
    while (true) {
        session_t* data = (session_t *)pcq_dequeue(pc_queue);
        thread_func = data->func;
        thread_func(data->arg);

        free(data->arg);
    }
    return NULL;
}*/

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
        //thread_array[i] = pthread_create(&thread_array[i], NULL, session_thread, NULL);
    }

    // unlink register_pipe_name if it already exists
    if (unlink(mbroker_config->register_pipe_name) < 0 && errno != ENOENT) {
        return -1;
    }

    // create the named pipe
    if (mkfifo(mbroker_config->register_pipe_name, 0666) < 0) {
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

/*
 * format:
 *  - mbroker <register_pipe_name> <max_sessions>
 */
int main(int argc, char **argv) {
    
    if (argc != 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    fprintf(stderr, "[INFO]: starting mbroker...\n");

    mbroker = (mbroker_t*) malloc(sizeof(mbroker_t));

    mbroker->register_pipe_name = argv[1]; // register_pipe_name is the name of the pipe to which the manager wants to connect to
    mbroker->max_sessions = (size_t) atoi(argv[2]);       // max_sessions is the maximum number of sessions that can be open at the same time
    
    // initialize the mbroker
    if (init_mbroker(mbroker) < 0) {
        fprintf(stderr, "failed: could not initialize mbroker\n");
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