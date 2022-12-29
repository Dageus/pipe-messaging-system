#include "logging.h"
#include "structures.h"
#include <stdlib.h>
#include <stdio.h>



int main(int argc, char **argv) {
    char *command = argv[0];

    // check arguments for each command
    // and whether or not the arguments are correct
    if (strcmp(command, "mbroker") == 0) {
        //check indiviual arguments
        if (argv[1] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* pipe_name = argv[1];
        if (argv[2] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        int max_sessions = atoi(argv[2]);
        // TODO: implement
        WARN("unimplemented");
    } else if (strcmp(command, "pub") == 0) {
        //check indiviual arguments
        if (argv[1] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* register_pipe = argv[1];
        if (argv[2] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* pipe_name = argv[2];
        if (argv[3] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* box_name = argv[3];
        // TODO: implement
        WARN("unimplemented");
    } else if (strcmp(command, "sub") == 0) {
        //check indiviual argumentsq
        if (argv[1] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* register_pipe = argv[1];
        if (argv[2] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* pipe_name = argv[2];
        if (argv[3] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* box_name = argv[3];
        // TODO: implement
        WARN("unimplemented");
    } else if (strcmp(command, "manager") == 0) {
        //check indiviual arguments
        if (argv[1] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* register_pipe = argv[1];
        if (argv[2] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* pipe_name = argv[2];
        if (argv[3] == NULL) {
            fprintf(stderr, "failed: missing argument for command '%s'\n", command);
            return -1;
        }
        char* action = argv[3];
        if (strcmp(action, "create")){
            if (argv[4] == NULL) {
                fprintf(stderr, "failed: missing argument for command '%s'\n", command);
                return -1;
            }
            char* box_name = argv[4];
            //TODO: implement   
        } else if (strcmp(action, "remove")){
            if (argv[4] == NULL) {
                fprintf(stderr, "failed: missing argument for command '%s'\n", command);
                return -1;
            }
            char* box_name = argv[4];
            //TODO: implement   
        } else if (strcmp(action, "list")){
            //TODO: implement   
        } else {
            fprintf(stderr, "failed: unknown command '%s'\n", command);
        }
        // TODO: implement
        WARN("unimplemented");
    } else {
        fprintf(stderr, "failed: unknown command '%s'\n", command);
    }

    char *pipe_name = argv[0]; // name of the named pipe
    int max_sessions = atoi(argv[1]); // number of threads to be created
    WARN("unimplemented"); // TODO: implement
    return -1;
}
