#include "logging.h"
#include <stdlib.h>

/*
    pipes work as files, use open to read whats in them, and write to them


    

*/



int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
    WARN("unimplemented"); // TODO: implement
    return -1;
}