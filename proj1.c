#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int tfs_open(char const *name, int flags){


    return 0;
}

int tfs_close(int fhandle){


    return 0;
}

ssize_t tfs_write(int fhandle, void const*buffer, size_t len){


    return 1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len){
    
    
    return 1;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path){

    FILE *f = fopen(source_path, "r");
    if (f == NULL){
        perror("File doesn't exist.\n");
        return -1;
    }
    // code goes here

    return 0;
}

int tfs_link(char const *target_file, char const *source_file){


    return 0;
}

int tfs_sym_link(char const *target_file, char const *source_file){


    return 0;
}

int tfs_unlink(char const *target){


    return 0;
}



int main(){


    return 0;
}