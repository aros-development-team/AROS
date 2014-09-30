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

char testfilename[] = "RAM:__TEST__";

int main() 
{
    struct stat buf;
    int fd;
    fd = creat(testfilename, 0700);
    TEST((fd != -1));
    close(fd);
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0700));
    TEST((chmod(testfilename, 0111) != -1));
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0111));
    TEST((chmod(testfilename, 0222) != -1));
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0222));
    TEST((chmod(testfilename, 0333) != -1));
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0333));
    TEST((chmod(testfilename, 0444) != -1));
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0444));
    TEST((chmod(testfilename, 0555) != -1));
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0555));
    TEST((chmod(testfilename, 0666) != -1));
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0666));
    TEST((chmod(testfilename, 0777) != -1));
    TEST((stat(testfilename, &buf) != -1));
    TEST(((buf.st_mode & 0777) == 0777));
    cleanup();
    return OK;
}

void cleanup() 
{
    remove(testfilename);
}
