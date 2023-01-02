#include "logging.h"
#include <stdlib.h>

/*
 * - the subscriber connects to the mbroker, citing the box name it wants to subscribe to
 *
 * - the mbroker checks if the box exists
 * 
 * - the subscriber scrapes the existing messages from the box and prints them to stdout
 * 
 * - the subscriber then waits for new messages to be published to the box
 * 
 * - prints new messages received to the box to the stdout
 * 
 * - to finish the session, the subscriber must receive a SIGINT (Ctrl+C), then closing the session and printing the number of messages received
 * 
 * - the named pipe is chosen by the subscriber
 * 
 * - the named pipe must be removed when the subscriber exits
 *
*/


/*
 * returns 0 on success, -1 on failure
 *
*/
int sub_to_box(char* named_pipe, char* box_name) {
    // open box_name
    // read from box_name
    // close box_name
    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
    WARN("unimplemented"); // TODO: implement
    return -1;
}