#include "logging.h"
#include "structures.h"
#include <stdio.h>


#define CLIENT_PIPE_PATH "/tmp/client_pipe_"

int client_pipe_num = 1;

// pipe path format: /tmp/client_pipe_<num>

/*
 *
 * - when the manager is launched, it connects to the mbroker
 * 
 * - it receives the answer from the mbroker in the SAME PIPE that the manager used to connect to the mbroker
 * 
 * - it prints the answer to the stdout, and then exits
 * 
 * - to create a pipe use the function mkfifo(<path>, <mode>)
 * 
 * - to use the pipe use the function open(<path>, <flags>)
 * 
 * - to remove a pipe use the function unlink(<path>)
 */


// delete structures.h

// manager creates named pipe for register


/*
 * returns 0 on success, -1 on failure
*/
int list_boxes(char* named_pipe) {
    // list boxes in mbroker
    // send this to the mbroker through the named pipe

    // receive the answer from the mbroker through the named pipe
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int create_box(char *named_pipe, char *box_name) {
    // create box in mbroker´

    //

    // example:
    //open(CLIENT_PIPE_PATH, O_RDONLY)

    // send this to the mbroker through the named pipe

    // receive the answer from the mbroker through the named pipe
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int remove_box(char *named_pipe, char *box_name) {
    // remove box from mbroker

    // send this to the mbroker through the named pipe

    // receive the answer from the mbroker through the named pipe
    return 0;
}


// register pipe name is the name of the pipe to which the manager wants to connect to

/*
 * format: 
 *  - manager <register_pipe_name> create <box_name>
 *  - manager <register_pipe_name> remove <box_name>
 *  - manager <register_pipe_name> list
 */
int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "failed: not enough arguments\n");
        return -1;
    }

    char *register_pipe_name = argv[1]; // assign the register pipe name
    char *command = argv[2];            // assign the command

    if (argc > 3){                      // this means we're either creating or removing a box
        char *box_name = argv[3];
        if (strcmp(command, "create") == 0) {
            create_box(register_pipe_name, box_name);
        } else if (strcmp(command, "remove") == 0) {
            remove_box(register_pipe_name, box_name);
        }
    } else {                            // this means we're listing the boxes
        list_boxes(register_pipe_name);
    }

    // more functions might be needed

    WARN("unimplemented"); // TODO: implement
    return -1;
}