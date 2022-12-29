
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    u_int8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
} publisher_register;

typedef struct {
    u_int8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
} subscriber_register;

typedef struct {
    u_int8_t code;
    char message[1024]; 
} publisher_message;

typedef struct {
    u_int8_t code;
    char message[1024]; 
} subscriber_message;

typedef struct {
    u_int8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
} box_request;

typedef struct {
    u_int8_t code;
    int32_t return_code; // 0 = success, -1 = error
    char error_message[1024]; // if return_code == -1 sends message, else send empty string
} box_response;

typedef struct {
    u_int8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
} box_removal;

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
    publisher_t *publisher;
    // TODO: Add list of subscriber_t
    publisher_message *message;
} box_t;

#endif