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
static const char *dangling = "T:lstat-dangling";
int fd = -1;

int main()
{
    struct stat st, st2;
    char other[sizeof(target)];
    unsigned int i;

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

    /* an ordinary object is the same object for stat() and lstat(),
       however its path is written */
    TEST((stat(target, &st) == 0));
    TEST((lstat(target, &st2) == 0));
    TEST((st2.st_dev == st.st_dev));
    TEST((st2.st_ino == st.st_ino));

    for (i = 0; i < sizeof(target); i++)
        other[i] = (target[i] >= 'a' && target[i] <= 'z')
            ? target[i] - 'a' + 'A' : target[i];
    TEST((lstat(other, &st2) == 0));
    TEST((st2.st_ino == st.st_ino));

    TEST((chdir("T:") == 0));
    TEST((lstat(target + 2, &st2) == 0));
    TEST((st2.st_ino == st.st_ino));

    /* a link whose target is missing is still a link */
    unlink(dangling);
    TEST((symlink("T:lstat-does-not-exist", dangling) == 0));
    TEST((stat(dangling, &st) == -1 && errno == ENOENT));
    TEST((lstat(dangling, &st) == 0));
    TEST((S_ISLNK(st.st_mode)));

    unlink(dangling);
    unlink(linkname);
    unlink(target);

    return OK;
}

void cleanup()
{
    if (fd != -1)
        close(fd);
    unlink(dangling);
    unlink(linkname);
    unlink(target);
}
