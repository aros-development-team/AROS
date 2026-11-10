/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    fsync() on descriptors open for writing, open only for reading, open on a
    directory, on descriptors that are not open, and on a descriptor owned by
    another fd.library subsystem (EINVAL, as for a socket).
*/

#include <proto/exec.h>
#include <proto/fd.h>
#include <libraries/fd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "test.h"

static const char testfilename[] = "T:__fsync_test__";
static int wfd = -1, rfd = -1, dfd = -1;

/* An owner no system component uses: its descriptors stand in for sockets */
#define TEST_OWNER ((fd_owner_t)(FD_OWNER_MAX - 1))
struct Library *FDBase;
static struct fd_hooks hooks;
static LONG ofd = -1;
static BOOL hooks_set;

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

    /* Open, but owned by another subsystem: EINVAL, not EBADF. */
    FDBase = OpenLibrary((CONST_STRPTR)"fd.library", 51);
    TEST((FDBase != NULL));
    TEST((FD_GetOwnerHooks(TEST_OWNER) == NULL));
    memset(&hooks, 0, sizeof(hooks));
    hooks.fdh_Version = FD_HOOKS_VERSION;
    TEST((FD_SetOwnerHooks(TEST_OWNER, &hooks) == 0));
    hooks_set = TRUE;
    TEST((FD_Alloc(32, TEST_OWNER, NULL, &ofd) == 0));
    errno = 0;
    TEST((fsync(ofd) == -1 && errno == EINVAL));

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
    if (ofd != -1)
        FD_Free(ofd, TEST_OWNER);
    ofd = -1;
    if (hooks_set)
        FD_SetOwnerHooks(TEST_OWNER, NULL);
    hooks_set = FALSE;
    if (FDBase)
        CloseLibrary(FDBase);
    FDBase = NULL;
    remove(testfilename);
}
