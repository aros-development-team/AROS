/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "test.h"

#define THREADS 4
#define ROUNDS  1500
#define NUMBERS 256

static volatile unsigned char held[NUMBERS];
static volatile int duplicates, failures;
static int saved_stdin = -1;

/* Note that the calling thread now owns fd. Returns 0 when another thread
   already holds the same number. */
static int take(int fd)
{
    if (fd < 0 || fd >= NUMBERS)
    {
        __atomic_fetch_add(&failures, 1, __ATOMIC_SEQ_CST);
        return 0;
    }
    if (__atomic_exchange_n(&held[fd], 1, __ATOMIC_SEQ_CST))
    {
        __atomic_fetch_add(&duplicates, 1, __ATOMIC_SEQ_CST);
        return 0;
    }
    return 1;
}

static void give(int fd)
{
    __atomic_store_n(&held[fd], 0, __ATOMIC_SEQ_CST);
}

static void use_file(const char *path, const char *text)
{
    char buf[32];
    size_t len = strlen(text);
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);

    if (!take(fd))
        return;
    if (write(fd, text, len) != (ssize_t)len
        || lseek(fd, 0, SEEK_SET) != 0
        || read(fd, buf, sizeof buf) != (ssize_t)len
        || memcmp(buf, text, len) != 0)
        __atomic_fetch_add(&failures, 1, __ATOMIC_SEQ_CST);
    give(fd);
    close(fd);
}

static void use_dir(void)
{
    DIR *dir = opendir("T:");
    int fd;

    if (!dir)
    {
        __atomic_fetch_add(&failures, 1, __ATOMIC_SEQ_CST);
        return;
    }
    fd = dirfd(dir);
    if (!take(fd))
        return;
    give(fd);
    closedir(dir);
}

static void use_pipe(void)
{
    int fds[2], got[2];

    if (pipe(fds) != 0)
    {
        __atomic_fetch_add(&failures, 1, __ATOMIC_SEQ_CST);
        return;
    }
    got[0] = take(fds[0]);
    got[1] = take(fds[1]);
    if (got[1])
    {
        give(fds[1]);
        close(fds[1]);
    }
    if (got[0])
    {
        give(fds[0]);
        close(fds[0]);
    }
}

static void *worker(void *arg)
{
    long id = (long)arg;
    char path[40], text[32];
    int i;

    snprintf(path, sizeof path, "T:fd_concurrent_%ld", id);
    snprintf(text, sizeof text, "thread %ld", id);

    for (i = 0; i < ROUNDS; i++)
    {
        switch ((i + id) % 3)
        {
            case 0: use_file(path, text); break;
            case 1: use_dir(); break;
            case 2: use_pipe(); break;
        }
    }
    unlink(path);

    return NULL;
}

void cleanup(void)
{
    if (saved_stdin != -1)
    {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
        saved_stdin = -1;
    }
}

int main(void)
{
    pthread_t threads[THREADS];
    long started, i;

    /* With number 0 free the threads also compete for a number that
       fd.library does not track */
    saved_stdin = dup(STDIN_FILENO);
    TEST(saved_stdin > STDERR_FILENO);
    held[STDOUT_FILENO] = held[STDERR_FILENO] = held[saved_stdin] = 1;
    TEST(close(STDIN_FILENO) == 0);

    for (started = 0; started < THREADS; started++)
    {
        if (pthread_create(&threads[started], NULL, worker, (void *)started) != 0)
            break;
    }
    for (i = 0; i < started; i++)
        pthread_join(threads[i], NULL);

    cleanup();

    TEST(started == THREADS);
    printf("%d threads, %d rounds: %d numbers given out twice, %d failed calls\n",
           THREADS, ROUNDS, duplicates, failures);
    TEST(duplicates == 0);
    TEST(failures == 0);

    return OK;
}
