/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <proto/dos.h>
#include "test.h"

int fd = -1;

int main()
{
    char path1[256];
    char path2[256];
    
    /* First something simple */
    TEST((chdir("SYS:") != -1));
    TEST((getcwd(path1, sizeof(path1)) != (char*) -1));
    TEST(((fd = open("SYS:", O_READ)) != -1));
    TEST((fchdir(fd) != -1));
    close(fd); fd = -1;
    TEST((getcwd(path2, sizeof(path2)) != (char*) -1));
    printf("Comparing paths: %s and %s\n", path1, path2);
    TEST((strcmp(path1, path2) == 0));

    /* Now more complicated case */
    TEST((mkdir("T:__TEST__", 0777) != -1));

    TEST((chdir("T:__TEST__") != -1));
    TEST((getcwd(path1, sizeof(path1)) != (char*) -1));
    TEST(((fd = open("T:__TEST__", O_READ)) != -1));
    TEST((fchdir(fd) != -1));
    close(fd); fd = -1;
    TEST((getcwd(path2, sizeof(path2)) != (char*) -1));
    printf("Comparing paths: %s and %s\n", path1, path2);
    TEST((strcmp(path1, path2) == 0));
    
    /* Test directory is going to disappear soon, evacuate! */
    TEST((chdir("SYS:") != -1));
    
    cleanup();
    return OK;
}

void cleanup()
{
    if(fd != -1)
	close(fd);
    
    DeleteFile("T:__TEST__");
}
