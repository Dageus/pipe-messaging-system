
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdio.h>
#include <stdlib.h>

#define CLIENT_PIPE_PATH "/tmp/client_pipe_"
#define SERVER_PIPE_PATH "/tmp/server_pipe"
#define BOX_MESSAGE_SIZE 296
#define BOX_LIST_MESSAGE_SIZE 264
#define MAX_MESSAGE_SIZE 1024

typedef struct {
    int max_sessions;
    char* register_pipe_path;
} mbroker_t;

typedef struct {
    char* box_name;
    
} box_t;

typedef struct {
    u_int8_t code;                       // code = 1
    char client_named_pipe_path[256];
    char box_name[32];
} publisher_register;

typedef struct {
    u_int8_t code;                       // code = 2
    char client_named_pipe_path[256];
    char box_name[32];
} subscriber_register;

typedef struct {
    u_int8_t code;                       // code = 3
    char client_named_pipe_path[256];
    char box_name[32];
} box_creation_request;

typedef struct {
    u_int8_t code;                       // code = 4
    int32_t return_code;                 // 0 = success, -1 = error
    char error_message[1024];            // if return_code == -1 sends message, else send empty string
} box_creation_response;

typedef struct {
    u_int8_t code;                       // code = 5
    char client_named_pipe_path[256];
    char box_name[32];
} box_removal_request;

typedef struct {
    u_int8_t code;                       // code = 6
    int32_t return_code;                 // 0 = success, -1 = error
    char error_message[1024];            // if return_code == -1 sends message, else send empty string
} box_removal_response;

typedef struct {
    u_int8_t code;                       // code = 7
    char client_named_pipe_path[256];
} box_list_request;

typedef struct {
    u_int8_t code;                       // code = 8
    u_int8_t return_code;                // 1 if it´s the last box, 0 if there are more boxes
    char box_name[32];                   // if return_code == 1 sends message, else send empty string
    u_int64_t box_size;                  // size of the box
    u_int64_t n_publishers;              // number of messages in the box
    u_int64_t n_subscribers;             // number of subscribers
} box_list_message;

// additional structs

typedef struct {
    u_int8_t code;
    char message[1024]; 
} publisher_message;

typedef struct {
    u_int8_t code;
    char message[1024]; 
} subscriber_message;


// Structure to hold the state of a subscriber client
typedef struct {
  int server_fd; // File descriptor for the connection to the mbroker server
  int client_fd; // File descriptor for the client's named pipe
  char box_name[32]; // Name of the message box the client is subscribed to
} subscriber_t;

// Structure to hold the state of a publisher client
typedef struct {
  int server_fd; // File descriptor for the connection to the mbroker server
  int client_fd; // File descriptor for the client's named pipe
  char box_name[32]; // Name of the message box the client is publishing to
} publisher_t;

typedef struct {
    publisher_t* publisher;
    subscriber_t* *subscribers;
    publisher_message* message;
} box_t;

// Structure to hold the state of the mbroker server
typedef struct {
  char* pipe_path;          // Path to the server's named pipe
  int max_sessions;         // Maximum number of concurrent sessions
} mbroker_t;

#endif