/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "test.h"

#define PRESENT_PATH "T:open_errno_present"
#define ABSENT_PATH "T:open_errno_absent"

int created = 0;

/* open() must fail and leave `expected` in the caller's errno, whatever an
   earlier call left there */
static int check(const char *path, int flags, int expected)
{
    int fd;

    errno = EDOM;
    fd = open(path, flags, 0644);
    if (fd != -1)
    {
        close(fd);
        return 0;
    }
    return errno == expected;
}

static int check_all(void)
{
    return check(ABSENT_PATH, O_RDONLY, ENOENT)
        && check(ABSENT_PATH, O_WRONLY, ENOENT)
        && check(PRESENT_PATH, O_CREAT | O_EXCL | O_WRONLY, EEXIST);
}

static void *worker(void *result)
{
    *(int *)result = check_all();
    return NULL;
}

int main(void)
{
    pthread_t thread;
    int worker_result = 0;
    int fd;

    unlink(ABSENT_PATH);
    fd = open(PRESENT_PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    TEST((fd != -1));
    created = 1;
    TEST((close(fd) == 0));

    TEST((check(ABSENT_PATH, O_RDONLY, ENOENT)));
    TEST((check(ABSENT_PATH, O_WRONLY, ENOENT)));
    TEST((check(PRESENT_PATH, O_CREAT | O_EXCL | O_WRONLY, EEXIST)));

    /* The result must reach the task that made the call */
    TEST((pthread_create(&thread, NULL, worker, &worker_result) == 0));
    TEST((pthread_join(thread, NULL) == 0));
    TEST((worker_result));

    cleanup();
    return OK;
}

void cleanup(void)
{
    if (created)
    {
        created = 0;
        unlink(PRESENT_PATH);
    }
}
