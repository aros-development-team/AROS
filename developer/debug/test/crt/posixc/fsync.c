/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    fsync() on descriptors open for writing, open only for reading, open on a
    directory, and on descriptors that are not open.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "test.h"

static const char testfilename[] = "T:__fsync_test__";
static int wfd = -1, rfd = -1, dfd = -1;

int main(void)
{
    static const char data[] = "fsync";

    wfd = open(testfilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    TEST((wfd != -1));
    TEST((write(wfd, data, sizeof(data)) == (ssize_t)sizeof(data)));
    TEST((fsync(wfd) == 0));
    TEST((close(wfd) == 0));
    wfd = -1;

    /* A read-only descriptor has nothing to write: success, not EBADF. */
    rfd = open(testfilename, O_RDONLY);
    TEST((rfd != -1));
    errno = 0;
    TEST((fsync(rfd) == 0));
    TEST((errno == 0));

    /* A directory, the usual way to sync a directory entry. */
    dfd = open("T:", O_RDONLY);
    TEST((dfd != -1));
    errno = 0;
    TEST((fsync(dfd) == 0));
    TEST((errno == 0));

    /* Not open: EBADF. */
    errno = 0;
    TEST((fsync(-1) == -1 && errno == EBADF));
    TEST((close(rfd) == 0));
    errno = 0;
    TEST((fsync(rfd) == -1 && errno == EBADF));
    rfd = -1;

    cleanup();
    return OK;
}

void cleanup(void)
{
    if (wfd != -1)
        close(wfd);
    if (rfd != -1)
        close(rfd);
    if (dfd != -1)
        close(dfd);
    wfd = rfd = dfd = -1;
    remove(testfilename);
}
