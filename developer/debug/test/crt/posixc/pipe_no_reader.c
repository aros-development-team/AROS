/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "test.h"

#define CHUNK 4096
#define LIMIT (16 * 1024 * 1024)
#define POLLS 300

static int fds[2] = { -1, -1 };
static volatile int finished;
static ssize_t last;
static int code;

static void *writer(void *unused)
{
    static char data[CHUNK];
    size_t total = 0;

    (void)unused;
    while (total < LIMIT)
    {
        last = write(fds[1], data, sizeof data);
        if (last <= 0)
            break;
        total += last;
    }
    code = errno;
    finished = 1;
    return NULL;
}

static int ended(void)
{
    int i;

    for (i = 0; i < POLLS && !finished; i++)
        usleep(10000);
    return finished;
}

int main(void)
{
    pthread_t thread;

    /* The writer blocks on a full pipe, then the read end closes. */
    TEST(pipe(fds) == 0);
    TEST(pthread_create(&thread, NULL, writer, NULL) == 0);
    usleep(200000);
    TEST(!finished);
    TEST(close(fds[0]) == 0);
    fds[0] = -1;
    TEST(ended());
    TEST(pthread_join(thread, NULL) == 0);
    TEST(last == -1);
    TEST(close(fds[1]) == 0);
    fds[1] = -1;

    /* The read end closes first, so the write fails without blocking. */
    finished = 0;
    TEST(pipe(fds) == 0);
    TEST(close(fds[0]) == 0);
    fds[0] = -1;
    TEST(pthread_create(&thread, NULL, writer, NULL) == 0);
    TEST(ended());
    TEST(pthread_join(thread, NULL) == 0);
    TEST(last == -1);

    return OK;
}

void cleanup(void)
{
    /* libpthread would wait at exit for a writer that never returns. */
    if (fds[1] != -1 && !finished)
        Exit(20);
    if (fds[0] != -1)
        close(fds[0]);
    if (fds[1] != -1)
        close(fds[1]);
}
