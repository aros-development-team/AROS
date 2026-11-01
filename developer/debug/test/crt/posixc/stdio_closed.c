/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Opening a private PosixC base must preserve closed standard descriptors.
    Every operation in the worker is finite; use an external test deadline
    to diagnose an unexpected library-open or pthread failure.
*/
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <libraries/posixc.h>
#include <proto/exec.h>
#include "test.h"

static int saved[3] = { -1, -1, -1 };
static int closed_mask;
static int worker_result;

void cleanup(void)
{
    int fd;
    for (fd = 0; fd < 3; fd++)
        if (saved[fd] >= 0)
        {
            dup2(saved[fd], fd);
            close(saved[fd]);
            saved[fd] = -1;
        }
}

static void *worker(void *unused)
{
    struct PosixCBase *base;
    FILE *streams[3];
    int fd;
    (void)unused;
    /* The pthread CRT hook has already opened a fresh private base. */
    base = (struct PosixCBase *)OpenLibrary((CONST_STRPTR)"posixc.library", 0);
    if (!base) { worker_result = 1; return NULL; }
    streams[0] = base->_stdin;
    streams[1] = base->_stdout;
    streams[2] = base->_stderr;
    for (fd = 0; fd < 3; fd++)
    {
        int result, error;
        if (!streams[fd] || fileno(streams[fd]) != fd)
        { worker_result = 2; break; }
        if (!(closed_mask & (1 << fd))) continue;
        errno = 0;
        result = fcntl(fd, F_GETFD);
        error = errno;
        if (result != -1 || error != EBADF)
        { worker_result = 3; break; }
        errno = 0;
        result = fd == 0 ? fgetc(streams[fd]) : fputc('X', streams[fd]);
        error = errno;
        if (result != EOF || error != EBADF)
        { worker_result = 4; break; }
    }
    CloseLibrary((struct Library *)base);
    return NULL;
}

int main(void)
{
    static const int masks[] = { 1, 2, 4, 7 };
    unsigned int mode;
    for (mode = 0; mode < sizeof(masks) / sizeof(masks[0]); mode++)
    {
        pthread_t thread;
        int fd, created, joined = -1, still_closed = 1;
        closed_mask = masks[mode];
        worker_result = 0;
        for (fd = 0; fd < 3; fd++)
            if (closed_mask & (1 << fd))
            {
                saved[fd] = fcntl(fd, F_DUPFD, 3);
                TEST(saved[fd] >= 3);
            }
        for (fd = 0; fd < 3; fd++)
            if (closed_mask & (1 << fd)) close(fd);
        created = pthread_create(&thread, NULL, worker, NULL);
        if (created == 0) joined = pthread_join(thread, NULL);
        for (fd = 0; fd < 3; fd++)
            if (closed_mask & (1 << fd))
            {
                int result, error;
                errno = 0;
                result = fcntl(fd, F_GETFD);
                error = errno;
                if (result != -1 || error != EBADF) still_closed = 0;
            }
        cleanup();
        printf("closed_mask=%d create=%d join=%d worker=%d preserved=%d\n",
            closed_mask, created, joined, worker_result, still_closed);
        TEST(created == 0);
        TEST(joined == 0);
        TEST(worker_result == 0);
        TEST(still_closed);
        errno = 0;
        TEST(fdopen(-1, "r") == NULL);
        TEST(errno == EBADF);
    }
    return OK;
}
