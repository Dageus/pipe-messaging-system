//This test verifies if the function tfs_copy_from_external_fs works correctly
//It copies the file "copy_file.txt" from the external file system to the tfs

#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char *str_ext_file = "The quick brown fox jumps.";
    char *path_copied_file = "/f1";
    char *path_src = "tests/copy_file.txt";
    char buffer[40];

    assert(tfs_init(NULL) != -1);

    ssize_t read;

    int status = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(status != -1);

    status = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(status != -1);

    read = tfs_read(status, buffer, sizeof(buffer) - 1);
    assert(read == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    // Repeat the copy to the same file
    status = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(status != -1);

    status = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(status != -1);

    // Contents should be overwriten, not appended
    read = tfs_read(status, buffer, sizeof(buffer) - 1);
    assert(read == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    printf("Successful test.\n");

    return 0;
}