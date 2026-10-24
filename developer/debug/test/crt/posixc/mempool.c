/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Memory allocated through one C runtime base and freed through another:
    in a pthread worker, by getline()/asprintf() and in a vfork() child.
    A free into the wrong pool traps or corrupts memory, but only traps when
    the block lies outside that pool, so the bases a worker opens are also
    checked for leaking: they used to keep a pool each.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <proto/exec.h>
#include "test.h"

struct result
{
    char *mem, *cwd, *str, *line;
    size_t linecap;
};

static char *volatile childcwd;

static void *worker(void *arg)
{
    struct result *r = arg;
    FILE *f;

    r->mem = malloc(100);
    r->cwd = getcwd(NULL, 0);
    asprintf(&r->str, "worker %d", 1);
    /* Grows the buffer main allocated */
    f = fopen("S:Startup-Sequence", "r");
    if (f)
    {
        getline(&r->line, &r->linecap, f);
        fclose(f);
    }
    return NULL;
}

static int round_trip(void)
{
    struct result r;
    pthread_t t;

    memset(&r, 0, sizeof(r));
    r.linecap = 1;
    r.line = malloc(r.linecap);
    if (pthread_create(&t, NULL, worker, &r) != 0)
        return 0;
    pthread_join(t, NULL);
    if (!(r.mem && r.cwd && r.str && r.line))
        return 0;
    free(r.mem);
    free(r.cwd);
    free(r.str);
    free(r.line);
    return 1;
}

int main(void)
{
    char *str = NULL;
    ULONG before, after;
    pid_t pid;
    int i, status;

    TEST(asprintf(&str, "main %d", 1) > 0);
    free(str);

    /* The first round sets up what stays for the program's lifetime */
    TEST(round_trip());
    before = AvailMem(MEMF_ANY);
    for (i = 0; i < 10; i++)
        TESTFALSE(round_trip());
    after = AvailMem(MEMF_ANY);
    printf("10 threads used %ld bytes\n", (long)before - (long)after);
    TEST((long)before - (long)after < 256 * 1024);

    pid = vfork();
    if (pid == 0)
    {
        childcwd = getcwd(NULL, 0);
        _exit(0);
    }
    TEST(pid > 0);
    waitpid(pid, &status, 0);
    TEST(childcwd != NULL);
    free(childcwd);

    return OK;
}

void cleanup(void)
{
}
