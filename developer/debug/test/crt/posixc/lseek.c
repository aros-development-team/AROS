/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include "test.h"

int fd;

int main()
{
    struct stat buf;
    int size = 12345;
    fd = mkstemp("T:lseekXXXXXX");
    
    TEST((lseek(fd, size, SEEK_SET) == size));
    TEST((fstat(fd, &buf) != -1));
    TEST((buf.st_size == size));

    TEST((lseek(fd, 0, SEEK_SET) == 0));
    TEST((lseek(fd, 100, SEEK_CUR) == 100));
    TEST((lseek(fd, -100, SEEK_CUR) == 0));

    TEST((lseek(fd, 0, SEEK_END) == size));
    TEST((lseek(fd, -size, SEEK_END) == 0));

    close(fd);
    return OK;
}

void cleanup()
{
    close(fd);
}
