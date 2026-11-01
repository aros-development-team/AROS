/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Finite vfork status regression, including a stdc opener without startup.
*/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <libraries/posixc.h>
#include <libraries/stdc.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "test.h"

extern struct PosixCBase *PosixCBase;

static struct StdCBase *original_stdc;
static struct Library *alternate_stdc;

static void restore_base(void)
{
    if (original_stdc != NULL)
    {
        PosixCBase->StdCBase = original_stdc;
        original_stdc = NULL;
    }
}

void cleanup(void)
{
    int i;

    restore_base();
    for (i = 0; i < 16 && waitpid(-1, NULL, WNOHANG) > 0; i++)
        ;
    if (alternate_stdc != NULL)
    {
        CloseLibrary(alternate_stdc);
        alternate_stdc = NULL;
    }
}

static long long milliseconds(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return -1;
    return (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

int main(void)
{
    volatile int mode;
    volatile int iteration;

    for (mode = 0; mode < 2; mode++)
    {
        for (iteration = 0; iteration < 8; iteration++)
        {
            pid_t child;
            pid_t waited;
            int status = -1;
            int saved_errno;
            volatile unsigned int polls = 0;
            long long now;
            volatile long long deadline = milliseconds();

            TEST(deadline >= 0);
            deadline += 5000;
            if (mode != 0)
            {
                alternate_stdc = OpenLibrary((CONST_STRPTR)"stdc.library", 0);
                TEST(alternate_stdc != NULL);
                /* A fresh per-opener base has not run program startup. */
                original_stdc = PosixCBase->StdCBase;
                PosixCBase->StdCBase = (struct StdCBase *)alternate_stdc;
            }

            child = vfork();
            if (child == 0)
                _exit(7 + mode);
            restore_base();
            TEST(child > 0);

            do
            {
                errno = 0;
                waited = waitpid(child, &status, WNOHANG);
                saved_errno = errno;
                now = milliseconds();
                if (waited != 0 || now < 0 || now >= deadline || ++polls >= 300)
                    break;
                Delay(1);
            } while (1);

            printf("mode=%d iteration=%d child=%ld waited=%ld errno=%d status=%d\n",
                mode, iteration, (long)child, (long)waited, saved_errno, status);
            TEST(waited == child);
            TEST(WIFEXITED(status));
            TEST(WEXITSTATUS(status) == 7 + mode);
            cleanup();
        }
    }
    return OK;
}
