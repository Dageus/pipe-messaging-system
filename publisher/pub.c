#include "logging.h"



/*
 * - publisher asks to join a server in the mbroker, citing the box name to which its gonna write to
 *
 * - mbroker creates a pipe for the publisher to write to
 * 
 * - if the connection is accepted by the mbroker, the client reads from the stdin and writes to the pipe
 * 
 * - each line corresponds to a message 
 * 
 * - it must not include a '\n' character, only the '\0' character
 * 
 * - if the publisher receives a SIGINT, it must close the pipe and exit
 * 
 * - the named pipe must be removed when the publisher exits
 * 
 */

/*
 * returns 0 on success, -1 on failure
*/
int pub_to_box(char *box_name) {
    // open box_name
    // write to box_name
    // close box_name
    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
    WARN("unimplemented"); // TODO: implement
    return -1;
}
