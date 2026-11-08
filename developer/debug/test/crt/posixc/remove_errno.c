/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "test.h"

#define BUSY_PATH "T:remove_errno_busy"
#define ABSENT_PATH "T:remove_errno_absent"
#define TARGET_PATH "T:remove_errno_target"

int fd = -1;

static int check(int (*call)(const char *), const char *path, int expected)
{
    errno = 0;
    if (call(path) != -1)
        return 0;
    return errno == expected;
}

static int check_rename(void)
{
    errno = 0;
    if (rename(ABSENT_PATH, TARGET_PATH) != -1)
        return 0;
    return errno == ENOENT;
}

static void *worker(void *result)
{
    *(int *)result = check(unlink, BUSY_PATH, EBUSY)
        && check(remove, BUSY_PATH, EBUSY)
        && check(unlink, ABSENT_PATH, ENOENT)
        && check_rename();
    return NULL;
}

int main(void)
{
    pthread_t thread;
    int worker_result = 0;

    fd = open(BUSY_PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    TEST((fd != -1));

    /* A file that is still open cannot be deleted on AROS */
    TEST((check(unlink, BUSY_PATH, EBUSY)));
    TEST((check(remove, BUSY_PATH, EBUSY)));
    TEST((check(unlink, ABSENT_PATH, ENOENT)));
    TEST((check_rename()));

    /* The result must reach the task that made the call */
    TEST((pthread_create(&thread, NULL, worker, &worker_result) == 0));
    TEST((pthread_join(thread, NULL) == 0));
    TEST((worker_result));

    cleanup();
    return OK;
}

void cleanup(void)
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
        unlink(BUSY_PATH);
    }
}
