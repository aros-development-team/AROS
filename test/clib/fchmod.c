/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "test.h"

char testfilename[] = "__TEST__";
int fd;

int main() 
{
    struct stat buf;
    fd = creat(testfilename, 0700);
    TEST((fd != -1));
    TEST((fchmod(fd, 0444) != -1));
    TEST((fstat(fd, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0444));
    TEST((fchmod(fd, 0555) != -1));
    TEST((fstat(fd, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0555));
    TEST((fchmod(fd, 0666) != -1));
    TEST((fstat(fd, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0666));
    TEST((fchmod(fd, 0777) != -1));
    TEST((fstat(fd, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0777));
    close(fd);
    cleanup();
    return OK;
}

void cleanup() 
{
    close(fd);
    remove(testfilename);
}
