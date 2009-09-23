#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "test.h"

char testfilename[] = "__TEST__";

int main()
{
    struct stat buf;
    int fd;
    fd = creat(testfilename, 0700);
    TEST((fd != -1));
    close(fd);

    TEST((stat(testfilename, &buf) != -1));
    printf("owner %d group %d\n", buf.st_uid, buf.st_gid);

    TEST((chown(testfilename, 1000, 1001) != -1));
    TEST((stat(testfilename, &buf) != -1));
    printf("owner %d group %d\n", buf.st_uid, buf.st_gid);

    TEST((chown(testfilename, 1001, 1000) != -1));
    TEST((stat(testfilename, &buf) != -1));
    printf("owner %d group %d\n", buf.st_uid, buf.st_gid);

    cleanup();
    return OK;
}

void cleanup() 
{
    remove(testfilename);
}
