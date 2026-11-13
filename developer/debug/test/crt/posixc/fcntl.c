/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
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
    const int modes[] = { O_RDONLY, O_WRONLY, O_RDWR };
    unsigned int i;
    char path[] = "T:fcntlXXXXXX";
    fd = mkstemp(path);
    TEST((fd != -1));

    /* Check if F_GETFD and F_SETFD is working */
    TEST((fcntl(fd, F_GETFD) == 0));
    TEST((fcntl(fd, F_SETFD, FD_CLOEXEC) == 0));
    TEST((fcntl(fd, F_GETFD) == FD_CLOEXEC));
    TEST((fcntl(fd, F_SETFD, 0) == 0));
    TEST((fcntl(fd, F_GETFD) == 0));
    
    fd2 = dup(fd);
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
    TEST((unlink(path) == 0));

    /* F_GETFL preserves access mode on original and duplicated descriptors. */
    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
    {
        int flags;

        fd = open("NIL:", modes[i]);
        TEST((fd != -1));
        fd2 = dup(fd);
        TEST((fd2 != -1));
        flags = fcntl(fd, F_GETFL);
        TEST((flags != -1));
        TEST(((flags & O_ACCMODE) == modes[i]));
        flags = fcntl(fd2, F_GETFL);
        TEST((flags != -1));
        TEST(((flags & O_ACCMODE) == modes[i]));
        cleanup();
    }

    return OK;
}

void cleanup()
{
    if(fd != -1)
    {
        close(fd);
        fd = -1;
    }
    if(fd2 != -1)
    {
        close(fd2);
        fd2 = -1;
    }
}
