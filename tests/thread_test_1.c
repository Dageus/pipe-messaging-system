#include "fs/operations.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define NUM_THREADS 4

// Function for the threads to run
void *thread_func(char* path_copied_file, char* path_src, char* buffer, char *str_ext_file) {


    int f;
    ssize_t r;

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    fprintf(stderr, "leu %ld bytes do ficheiro depois de escrever lá o ext_file\n", r);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    return NULL;
}

int main() {
    char *str_ext_file =
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! ";
    char *path_copied_file = "/f1";
    char *path_src = "tests/file_to_copy_over512.txt";
    char buffer[600];

    assert(tfs_init(NULL) != -1);
    pthread_t threads[NUM_THREADS];

    // Create and run threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int ret = pthread_create(&threads[i], NULL, thread_func(path_copied_file, path_src, buffer, str_ext_file), &i);
        assert(ret == 0);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        int ret = pthread_join(threads[i], NULL);
        assert(ret == 0);
    }

    printf("Successful test.\n");
}