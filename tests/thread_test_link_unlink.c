//include all the necessary files
#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

const char write_contents[] = "Hello World!";
char read_contents[sizeof(write_contents)];

void *thread_func_write() {
    char *file_path = "/f1";

    int f;
    ssize_t r;

    f = tfs_open(file_path, TFS_O_CREAT);
    assert(f != -1);
    
    //write to the file
    r = tfs_write(f, write_contents, strlen(write_contents));
    //check if the write was successful
    assert(r == strlen(write_contents));

    assert(tfs_close(f) != -1);
    return  NULL;
}

void *thread_func_read () {
    char *file_path = "/f1";

    int f;
    ssize_t r;

    f = tfs_open(file_path, 0);
    assert(f != -1);

    //read from the file and check if the read was successful
    while( (r = tfs_read(f, read_contents, sizeof(read_contents) - 1)) > 0) {
        read_contents[r] = '\0';
        assert(r == strlen(read_contents));
    }
       
    assert(tfs_close(f) != -1);
    return NULL;

}

void *thread_func_link() {
    char *file_path = "/f1";
    char *link_path = "/f2";

    assert(tfs_link(file_path, link_path) != -1);

    return  NULL;
}

void *thread_func_unlink() {
    char *file_path = "/f1";
    
    assert(tfs_unlink(file_path) != -1);

    return NULL;
}


int main(){
    pthread_t t_link;
    pthread_t t_unlink;
    pthread_t t_write;
    pthread_t t_read;

    //initialize the file system
    assert(tfs_init(NULL) != -1);

    //create the threads
    assert(pthread_create(&t_link, NULL, thread_func_link, NULL) == 0);
    assert(pthread_create(&t_unlink, NULL, thread_func_unlink, NULL) == 0);
    assert(pthread_create(&t_write, NULL, thread_func_write, NULL) == 0);
    assert(pthread_create(&t_read, NULL, thread_func_read, NULL) == 0);

    //join the threads
    assert(pthread_join(t_write, NULL) == 0);
    assert(pthread_join(t_link, NULL) == 0);
    assert(pthread_join(t_unlink, NULL) == 0);
    assert(pthread_join(t_read, NULL) == 0);


    assert(strcmp(read_contents, write_contents) == 0);

    //destroy the file system
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}