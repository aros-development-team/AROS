/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    opendir() must fail cleanly when posixc cannot provide a descriptor slot.
    Needs posixc.library built with failure points:

        make compiler-posixc POSIXC_FAULT_INJECTION=yes

    The test sets its own local variable "posixc_fault": "probe" first, which
    such a build consumes without failing, to learn whether failure points
    exist; then "grow" (growing the descriptor array fails) or "reserve" (the
    number is refused as fd.library would refuse it, EMFILE). posixc fails at
    that point once and deletes the variable. Nothing is patched and no other
    task is affected. Only when "probe" is not consumed (a regular posixc)
    does the test skip; once failure points are known to exist, a point that
    is not reached is a test failure.

    The test creates its two drawers under names of its own, refuses to run
    if either exists, and deletes only what it created.
        opendir_slotfail [grow|reserve|selfcheck] [NAME]
    "grow" or "reserve" runs only that case. "selfcheck" arms a point that
    posixc does not have and expects the test to report it as not reached
    and fail (return code 5), never to skip. NAME replaces the generated base
    name (the second drawer is NAME_held), for checking the refusal.
*/

#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/var.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

#define ROUNDS 20
#define HELD 8      /* drawers kept open at most to push the number up */

static DIR *held[HELD];
static int nheld;
static char testdir[64], helddir[80];
static int created_test, created_held;

static int armed(void)
{
    char value[16];
    return GetVar("posixc_fault", value, sizeof value, GVF_LOCAL_ONLY) > 0;
}

static int arm(const char *point)
{
    if (!SetVar("posixc_fault", point, -1, GVF_LOCAL_ONLY))
    {
        printf("cannot set posixc_fault (%ld)\n", (long)IoErr());
        return 0;
    }
    return armed();
}

/* The drawer is unlocked only if opendir() released its lock. */
static int unlocked(void)
{
    BPTR lock = Lock(testdir, EXCLUSIVE_LOCK);

    if (!lock)
        return 0;
    UnLock(lock);
    return 1;
}

/* One armed opendir(). Returns -1 when the point was not reached (the
   variable is still there; it is removed), 0 on a wrong result, 1 when
   opendir() failed with the expected errno and left no lock behind. */
static int injected(const char *point, int expect)
{
    DIR *dir;

    if (!arm(point))
        return 0;
    errno = 0;
    dir = opendir(testdir);
    if (armed())
    {
        DeleteVar("posixc_fault", GVF_LOCAL_ONLY);
        if (dir)
            closedir(dir);
        return -1;
    }
    if (dir)
    {
        /* Not closed: with descriptor -1, closedir() would act on memory
           before the descriptor array again. */
        printf("opendir returned %p with descriptor %d after an injected failure\n",
               dir, dirfd(dir));
        return 0;
    }
    if (errno != expect)
    {
        printf("errno %d, expected %d\n", errno, expect);
        return 0;
    }
    return unlocked();
}

/* The array grows only when the next number lies above it, and a number
   freed by another descriptor may be reused instead. Keep one more drawer
   open before each try, so that the number rises, a bounded number of
   times. */
static int injected_grow(const char *point, int expect)
{
    for (;;)
    {
        int r = injected(point, expect);
        if (r != -1 || nheld == HELD)
            return r;
        if (!(held[nheld] = opendir(helddir)))
            return 0;
        nheld++;
    }
}

/* Failure points are known to exist; not reaching one is a failure. */
static int reached(const char *point, int r)
{
    if (r == -1)
        printf("failure point %s not reached\n", point);
    return r == 1;
}

static int repeated(const char *point, int expect)
{
    ULONG mem = AvailMem(MEMF_ANY);
    int i;

    for (i = 1; i < ROUNDS; i++)
        if (!reached(point, injected(point, expect)))
            return 0;
    printf("memory after %d more %s failures: %ld bytes difference\n",
           ROUNDS - 1, point, (long)AvailMem(MEMF_ANY) - (long)mem);
    return (long)mem - (long)AvailMem(MEMF_ANY) < 4096;
}

/* Create a drawer that must not exist yet; remember that it is ours. */
static int create_own(const char *name, int *created)
{
    BPTR lock = Lock(name, ACCESS_READ);

    if (lock)
    {
        UnLock(lock);
        printf("%s exists; refusing to use it\n", name);
        return 0;
    }
    lock = CreateDir(name);
    if (!lock)
    {
        printf("cannot create %s (%ld)\n", name, (long)IoErr());
        return 0;
    }
    UnLock(lock);
    *created = 1;
    return 1;
}

int main(int argc, char **argv)
{
    DIR *dir;
    struct dirent *de;
    int fd, i, grow = 1, reserve = 1, selfcheck = 0;
    const char *base = NULL;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "grow"))
            reserve = 0;
        else if (!strcmp(argv[i], "reserve"))
            grow = 0;
        else if (!strcmp(argv[i], "selfcheck"))
            selfcheck = 1;
        else
            base = argv[i];
    }
    if (base)
        snprintf(testdir, sizeof testdir, "%s", base);
    else
        snprintf(testdir, sizeof testdir, "RAM:opendir_slotfail_%lx",
                 (unsigned long)(IPTR)FindTask(NULL));
    snprintf(helddir, sizeof helddir, "%s_held", testdir);
    TEST(create_own(testdir, &created_test));
    TEST(create_own(helddir, &created_held));

    /* Does this posixc have failure points? "probe" fails nothing. */
    TEST(arm("probe"));
    dir = opendir(testdir);
    TEST(dir != NULL);
    TEST(closedir(dir) == 0);
    if (armed())
    {
        printf("posixc.library has no failure points "
               "(build it with POSIXC_FAULT_INJECTION=yes): test skipped\n");
        cleanup();
        return OK;
    }

    if (selfcheck)
    {
        /* Regression: an unreached point must fail the test, not skip it. */
        TEST(reached("selfcheck-unknown", injected_grow("selfcheck-unknown", ENOMEM)));
        cleanup();
        return OK;
    }

    /* fd.library refuses the number: its error is kept. */
    if (reserve)
    {
        TEST(reached("reserve", injected("reserve", EMFILE)));
        TEST(repeated("reserve", EMFILE));
    }

    /* Growing the array fails: ENOMEM. */
    if (grow)
    {
        TEST(reached("grow", injected_grow("grow", ENOMEM)));
        TEST(repeated("grow", ENOMEM));
    }

    /* Nothing armed: opendir() works and the descriptor is real. */
    TEST(!armed());
    dir = opendir(testdir);
    TEST(dir != NULL);
    fd = dirfd(dir);
    TEST(fd > STDERR_FILENO && fcntl(fd, F_GETFD) != -1);
    errno = 0;
    while ((de = readdir(dir)))
        ;
    TEST(errno == 0);
    TEST(closedir(dir) == 0);
    TEST(fcntl(fd, F_GETFD) == -1 && errno == EBADF);

    cleanup();
    return OK;
}

void cleanup(void)
{
    DeleteVar("posixc_fault", GVF_LOCAL_ONLY);
    while (nheld > 0)
        closedir(held[--nheld]);
    /* Only what this run created, so an existing object is never removed. */
    if (created_held && DeleteFile(helddir))
        created_held = 0;
    if (created_test && DeleteFile(testdir))
        created_test = 0;
}
