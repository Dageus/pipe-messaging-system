#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

/*
 *
 * - when the manager is launched, it connects to the mbroker
 * 
 * - it receives the answer from the mbroker in the SAME PIPE that the manager used to connect to the mbroker
 * 
 * - it prints the answer to the stdout, and then exits
 * 
 */


//

// manager creates named pipe for register


/*
 * returns 0 on success, -1 on failure
*/
int list_boxes() {
    // list boxes in mbroker
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int create_box(char *box_name) {
    // create box in mbroker
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int remove_box(char *box_name) {
    // remove box from mbroker
    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    print_usage();
    WARN("unimplemented"); // TODO: implement
    return -1;
}
