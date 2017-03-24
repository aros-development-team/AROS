/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test.h"

int fd = -1;
int fd2 = -1;

int main()
{
    fd = mkstemp("T:lseekXXXXXX");
    TEST((fd != -1));

    /* Check if F_GETFD and F_SETFD is working */
    TEST((fcntl(fd, F_GETFD) == 0));
    TEST((fcntl(fd, F_SETFD, FD_CLOEXEC) == 0));
    TEST((fcntl(fd, F_GETFD) == FD_CLOEXEC));
    TEST((fcntl(fd, F_SETFD, 0) == 0));
    TEST((fcntl(fd, F_GETFD) == 0));
    
    int fd2 = dup(fd);
    TEST((fd2 != -1));

    /* Check if descriptor flags are independent for duped descriptors */
    TEST((fcntl(fd2, F_GETFD) == 0));
    TEST((fcntl(fd2, F_SETFD, FD_CLOEXEC) == 0));
    TEST((fcntl(fd2, F_GETFD) == FD_CLOEXEC));
    TEST((fcntl(fd, F_GETFD) == 0));
    TEST((fcntl(fd, F_SETFD, FD_CLOEXEC) == 0));
    TEST((fcntl(fd2, F_SETFD, 0) == 0));
    TEST((fcntl(fd, F_GETFD) == FD_CLOEXEC));
    
    cleanup();
    return OK;
}

void cleanup()
{
    if(fd != -1)
	close(fd);
    if(fd2 != -1)
	close(fd2);
}
