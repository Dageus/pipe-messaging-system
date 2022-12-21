#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

const char *file_contents = "Hello World!";
const char *path = "/f1";

void *thread_func_write() {

    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    
    assert(tfs_write(f, file_contents, sizeof(file_contents)) == sizeof(file_contents));

    assert(tfs_close(f) != -1);
    return  NULL;
}

void *thread_func_read () {
    char buffer[sizeof(file_contents)];

    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(file_contents));
       
    assert(tfs_close(f) != -1);
    return NULL;
}

int main() {

    pthread_t tread_1;
    pthread_t tread_2;
    pthread_t twrite_1;
    pthread_t twrite_2;

    
    assert(tfs_init(NULL) != -1);

    assert(pthread_create(&twrite_1, NULL, thread_func_write, NULL) == 0);
    assert(pthread_create(&twrite_2, NULL, thread_func_write, NULL) == 0);
    assert(pthread_create(&tread_1, NULL, thread_func_read, NULL) == 0);
    assert(pthread_create(&tread_2, NULL, thread_func_read, NULL) == 0);

    assert(pthread_join(twrite_1, NULL) == 0);
    assert(pthread_join(twrite_2, NULL) == 0);
    assert(pthread_join(tread_1, NULL) == 0);
    assert(pthread_join(tread_2, NULL) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
