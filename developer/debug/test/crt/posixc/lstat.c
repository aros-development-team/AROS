/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    lstat() must describe a soft link itself, where stat() describes its
    target.
*/

#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "test.h"

static char target[] = "T:lstatXXXXXX";
static const char *linkname = "T:lstat-link";
int fd = -1;

int main()
{
    struct stat st;

    fd = mkstemp(target);
    TEST((fd != -1));
    TEST((write(fd, "0123456789", 10) == 10));
    close(fd);
    fd = -1;

    unlink(linkname);
    TEST((symlink(target, linkname) == 0));

    /* stat() follows the link */
    TEST((stat(linkname, &st) == 0));
    TEST((S_ISREG(st.st_mode)));
    TEST((st.st_size == 10));

    /* lstat() does not */
    TEST((lstat(linkname, &st) == 0));
    TEST((S_ISLNK(st.st_mode)));

    /* and still describes ordinary objects */
    TEST((lstat(target, &st) == 0));
    TEST((S_ISREG(st.st_mode)));
    TEST((st.st_size == 10));
    TEST((lstat("T:", &st) == 0));
    TEST((S_ISDIR(st.st_mode)));
    TEST((lstat("T:lstat-does-not-exist", &st) == -1 && errno == ENOENT));

    unlink(linkname);
    unlink(target);

    return OK;
}

void cleanup()
{
    if (fd != -1)
        close(fd);
    unlink(linkname);
    unlink(target);
}
