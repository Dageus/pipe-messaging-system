#include "logging.h"
#include "structures.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

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


//

// manager creates named pipe for register


/*
 * returns 0 on success, -1 on failure
*/
int list_boxes(char* named_pipe) {
    // list boxes in mbroker
    box_list_request request = {
        .code = 1,
        .client_named_pipe_path = named_pipe
    };

    // send this to the mbroker through the named pipe

    // receive the answer from the mbroker through the named pipe
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int create_box(char *named_pipe, char *box_name) {
    // create box in mbroker
    box_creation_request request = {
        .code = 2,
        .box_name = box_name,
        .client_named_pipe_path = named_pipe
    };

    // send this to the mbroker through the named pipe

    // receive the answer from the mbroker through the named pipe
    return 0;
}

/*
 * returns 0 on success, -1 on failure
*/
int remove_box(char *named_pipe, char *box_name) {
    // remove box from mbroker
    box_removal_request request = {
        .code = 3,
        .box_name = box_name,
        .client_named_pipe_path = named_pipe
    };

    // send this to the mbroker through the named pipe

    // receive the answer from the mbroker through the named pipe
    return 0;
}

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
