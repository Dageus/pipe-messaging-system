#include "logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_MESSAGE_SIZE 1024

// Structure to hold the state of a subscriber client
typedef struct {
  int server_fd; // File descriptor for the connection to the mbroker server
  int client_fd; // File descriptor for the client's named pipe
  char message_box[256]; // Name of the message box the client is subscribed to
} subscriber_t;

// Function prototypes
int subscriber_connect(subscriber_t *subscriber, const char *server_pipe, const char *client_pipe);
int subscriber_subscribe(subscriber_t *subscriber, const char *message_box);
int subscriber_receive(subscriber_t *subscriber, char *buffer, size_t size);
int subscriber_disconnect(subscriber_t *subscriber);

// Implementation of the subscriber_connect function
int subscriber_connect(subscriber_t *subscriber, const char *server_pipe, const char *client_pipe) {
    // Create the client's named pipe
    if (mkfifo(client_pipe, 0666) < 0 && errno != EEXIST) {
      perror("Error creating named pipe");
      return -1;
    }   

    // Open the client's named pipe for reading
    subscriber->client_fd = open(client_pipe, O_RDONLY | O_NONBLOCK);
    if (subscriber->client_fd < 0) {
        perror("Error opening named pipe for reading");
        return -1;
    }

    // Open the mbroker server's named pipe for writing
    subscriber->server_fd = open(server_pipe, O_WRONLY);
    if (subscriber->server_fd < 0) {
        perror("Error opening named pipe for writing");
        close(subscriber->client_fd);
        return -1;
    }

    return 0;
}

// Implementation of the subscriber_subscribe function
int subscriber_subscribe(subscriber_t *subscriber, const char *message_box) {
    // Send a message to the mbroker server requesting to subscribe to the message box
    if (write(subscriber->server_fd, "SUBSCRIBE", 10) < 0 ||
        write(subscriber->server_fd, message_box, strlen(message_box) + 1) < 0) {
        perror("Error writing to named pipe");
        return -1;
    }

        // Save the name of the message box in the subscriber structure
        strncpy(subscriber->message_box, message_box, sizeof(subscriber->message_box));

    return 0;
}

// Implementation of the subscriber_receive function
int subscriber_receive(subscriber_t *subscriber, char *buffer, size_t size) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(subscriber->client_fd, &read_fds);

    // Wait for data to be available on the client's named pipe
    if (select(subscriber->client_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
        perror("Error in select");
        return -1;
    }

    // Read the message from the client's named pipe
    ssize_t n = read(subscriber->client_fd, buffer, size);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, return 0
            return 0;
        }
    }
}


int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
    WARN("unimplemented"); // TODO: implement
    return -1;
}