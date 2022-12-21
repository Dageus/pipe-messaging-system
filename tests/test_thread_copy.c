//This test verifies if the function tfs_copy_from_external_fs works correctly when used with threads.
//It copies the file "copy_file.txt" from the external file system to the tfs

#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

void *thread_func_copy() {
    
    char *path_copied_file = "/f1";
    char *path_src = "tests/copy_file.txt";

    int status = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(status != -1);

    return NULL;
}


int main() {

    char *str_ext_file = "The quick brown fox jumps.";
    char *path_copied_file = "/f1";
    char buffer[40];


    
    assert(tfs_init(NULL) != -1);

    //creat a thread and copy the file to another path
    pthread_t thread_1;
    pthread_t thread_2;
    pthread_t thread_3;
    
    assert(pthread_create(&thread_1, NULL, thread_func_copy, NULL) == 0);
    assert(pthread_create(&thread_2, NULL, thread_func_copy, NULL) == 0);
    assert(pthread_create(&thread_3, NULL, thread_func_copy, NULL) == 0);

    assert(pthread_join(thread_1, NULL) == 0);
    assert(pthread_join(thread_2, NULL) == 0);
    assert(pthread_join(thread_3, NULL) == 0);
    
    int status = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(status != -1);
    
    ssize_t read;

    read = tfs_read(status, buffer, sizeof(buffer) - 1);
    assert(read == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    printf("Successful test.\n");

    return 0;
}