
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>


#define BOX_MESSAGE_SIZE 289
#define BOX_LIST_MESSAGE_SIZE 257
#define MAX_MESSAGE_SIZE 1024
#define ANSWER_MESSAGE_SIZE 1029
#define MAX_BOX_NAME_SIZE 32
#define MAX_NAMED_PIPE_SIZE 256

// op_codes
#define OP_CODE_REGISTER_PUBLISHER  1
#define OP_CODE_REGISTER_SUBSCRIBER 2
#define OP_CODE_REGISTER_BOX        3
#define OP_CODE_ANSWER_TO_CREATION  4
#define OP_CODE_REMOVE_BOX          5
#define OP_CODE_ANSWER_TO_REMOVAL   6
#define OP_CODE_BOX_LIST            7
#define OP_CODE_ANSWER_TO_LIST      8
#define OP_CODE_PUBLISHER_MESSAGE   9
#define OP_CODE_SUBSCRIBER_MESSAGE  10


// Structure to hold the state of a subscriber client
typedef struct {
    int box_fd;              // File descriptor for the client's named pipe
    char named_pipe[256];        
    char box_name[32];       // Name of the message box the client is subscribed to
} subscriber_t;

// Structure to hold the state of a publisher client
typedef struct {
    char named_pipe[256];   // File descriptor for the client's named pipe
    char* box_name;         // Name of the message box the client is subscribed to
} publisher_t;

typedef struct subscriber_list_t{
    subscriber_t* subscriber;
    struct subscriber_list_t* next;
} subscriber_list_t;

typedef struct message_list_t{
    size_t message_size;
    struct message_list_t* next;
} message_list_t;

typedef struct {
    char* box_name;
    subscriber_list_t* subscribers;
    char *publisher_named_pipe;
    u_int64_t num_subscribers;
    message_list_t* messages_size;
    pthread_cond_t box_cond;
} box_t;

typedef struct box_list_t{
    box_t* box;
    struct box_list_t* next;
} box_list_t;

// Structure to hold the state of the mbroker server
typedef struct {
    char* register_pipe_name;               // Path to the server's named pipe
    size_t max_sessions;                    // Maximum number of concurrent sessions
} mbroker_t;

typedef struct {
    char request[MAX_MESSAGE_SIZE];
    int pipe_fd;
} session_t;

#endif