/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    flock() locks belong to the open file description: a second open() of the
    same file conflicts with the first, also in the same task; LOCK_UN only
    removes the lock of the given description; closing the last descriptor
    of a description, or the exit of the program, releases its lock.

    Another process is the program itself, started through the Shell with an
    argument: "try FILE" and "trysh FILE" report with their return code
    whether an exclusive or shared non-blocking lock could be obtained (and
    release it again), "hold FILE" locks FILE and exits without unlocking or
    closing it.

    Every case uses its own file in RAM:, named after this task. An existing
    file is refused, and only the files created here are deleted. A case does
    not unlock a lock it cannot be sure to hold, so that a library with the
    old behaviour runs to the end.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>

#include <sys/file.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

#define CHILD_GOT       0
#define CHILD_BLOCKED   5
#define CHILD_ERROR     10

#define CASES 6

static char self[512];
static char files[CASES][64];
static BOOL created[CASES];
static int failures;

static void check(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "pass" : "FAIL", what);
    if (!ok)
        failures++;
}

static int child_main(const char *op, const char *file)
{
    int fd = open(file, O_RDONLY);

    if (fd == -1)
        return CHILD_ERROR;

    if (!strcmp(op, "hold"))
        return flock(fd, LOCK_EX | LOCK_NB) == 0 ? CHILD_GOT : CHILD_ERROR;

    if (flock(fd, (!strcmp(op, "trysh") ? LOCK_SH : LOCK_EX) | LOCK_NB) == 0)
    {
        flock(fd, LOCK_UN);
        close(fd);
        return CHILD_GOT;
    }
    close(fd);
    return errno == EWOULDBLOCK ? CHILD_BLOCKED : CHILD_ERROR;
}

static LONG child(const char *op, int c)
{
    char cmd[700];

    snprintf(cmd, sizeof(cmd), "\"%s\" %s \"%s\"", self, op, files[c]);
    return SystemTags((CONST_STRPTR)cmd, SYS_Output, Output(), TAG_END);
}

static int create_file(int c)
{
    int fd;

    snprintf(files[c], sizeof(files[c]), "RAM:flock-description-%lx-%d",
             (unsigned long)(IPTR)FindTask(NULL), c);
    fd = open(files[c], O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1)
    {
        printf("FAIL: cannot create %s (exists or error %d)\n", files[c], errno);
        failures++;
        return -1;
    }
    created[c] = TRUE;
    return fd;
}

static int reopen(int c)
{
    return open(files[c], O_RDONLY);
}

/* Two descriptions of one file in one task */
static void case_same_task(void)
{
    int fd1 = create_file(0), fd2;

    if (fd1 == -1)
        return;
    fd2 = reopen(0);
    check(fd2 != -1, "same task: second open");
    check(flock(fd1, LOCK_EX) == 0, "same task: exclusive lock on first descriptor");
    errno = 0;
    check(flock(fd2, LOCK_EX | LOCK_NB) == -1 && errno == EWOULDBLOCK,
          "same task: exclusive LOCK_NB on second descriptor fails with EWOULDBLOCK");
    errno = 0;
    check(flock(fd2, LOCK_SH | LOCK_NB) == -1 && errno == EWOULDBLOCK,
          "same task: shared LOCK_NB on second descriptor fails with EWOULDBLOCK");
    errno = 0;
    check(flock(fd2, LOCK_EX) == -1 && errno == EDEADLK,
          "same task: blocking exclusive lock on second descriptor fails with EDEADLK");
    close(fd2);
    close(fd1);
}

/* LOCK_UN on a descriptor that holds no lock */
static void case_unlock_other(void)
{
    int fd1 = create_file(1), fd2;

    if (fd1 == -1)
        return;
    fd2 = reopen(1);
    check(flock(fd1, LOCK_EX) == 0, "unlock: exclusive lock on first descriptor");
    check(flock(fd2, LOCK_UN) == 0, "unlock: LOCK_UN on second descriptor succeeds");
    check(child("try", 1) == CHILD_BLOCKED,
          "unlock: another process is still refused afterwards");
    close(fd2);
    close(fd1);
}

/* Closing releases the lock once the last descriptor of the description is
   gone */
static void case_close(void)
{
    int fd1 = create_file(2), fd3;

    if (fd1 == -1)
        return;
    check(flock(fd1, LOCK_EX) == 0, "close: exclusive lock");
    fd3 = dup(fd1);
    check(fd3 != -1, "close: dup");
    close(fd1);
    check(child("try", 2) == CHILD_BLOCKED,
          "close: lock kept while a dup()ed descriptor is open");
    close(fd3);
    check(child("try", 2) == CHILD_GOT,
          "close: lock released when the last descriptor is closed");
}

/* Program exit releases the lock */
static void case_exit(void)
{
    int fd1 = create_file(3);

    if (fd1 == -1)
        return;
    check(child("hold", 3) == CHILD_GOT, "exit: another process locked the file and exited");
    errno = 0;
    check(flock(fd1, LOCK_EX | LOCK_NB) == 0, "exit: lock released by the exit");
    close(fd1);
}

/* Shared locks of two descriptions */
static void case_shared(void)
{
    int fd1 = create_file(4), fd2;

    if (fd1 == -1)
        return;
    fd2 = reopen(4);
    check(flock(fd1, LOCK_SH) == 0, "shared: first descriptor");
    check(flock(fd2, LOCK_SH | LOCK_NB) == 0, "shared: second descriptor");
    check(child("trysh", 4) == CHILD_GOT, "shared: another process gets a shared lock");
    check(child("try", 4) == CHILD_BLOCKED, "shared: another process is refused an exclusive lock");
    check(flock(fd1, LOCK_UN) == 0, "shared: unlock first descriptor");
    check(child("try", 4) == CHILD_BLOCKED, "shared: second descriptor still holds its lock");
    check(flock(fd2, LOCK_UN) == 0, "shared: unlock second descriptor");
    check(child("try", 4) == CHILD_GOT, "shared: file free after both unlocks");
    close(fd2);
    close(fd1);
}

/* Converting the lock of one description */
static void case_convert(void)
{
    int fd1 = create_file(5);

    if (fd1 == -1)
        return;
    check(flock(fd1, LOCK_SH) == 0, "convert: shared lock");
    check(flock(fd1, LOCK_EX | LOCK_NB) == 0, "convert: upgrade to exclusive");
    check(child("trysh", 5) == CHILD_BLOCKED, "convert: another process is refused a shared lock");
    check(flock(fd1, LOCK_SH) == 0, "convert: downgrade to shared");
    check(child("trysh", 5) == CHILD_GOT, "convert: another process gets a shared lock");
    close(fd1);
    check(child("try", 5) == CHILD_GOT, "convert: file free after close");
}

int main(int argc, char **argv)
{
    BPTR dir;
    char name[256];

    if (argc == 3)
        return child_main(argv[1], argv[2]);

    dir = GetProgramDir();
    if (!dir || !NameFromLock(dir, (STRPTR)self, sizeof(self)) ||
        !GetProgramName((STRPTR)name, sizeof(name)) ||
        !AddPart((STRPTR)self, FilePart((CONST_STRPTR)name), sizeof(self)))
    {
        printf("FAIL: cannot find the program's own path\n");
        return FAIL;
    }

    case_same_task();
    case_unlock_other();
    case_close();
    case_exit();
    case_shared();
    case_convert();

    cleanup();
    printf("%d failure(s)\n", failures);
    return failures ? FAIL : OK;
}

void cleanup(void)
{
    int c;

    for (c = 0; c < CASES; c++)
        if (created[c])
        {
            remove(files[c]);
            created[c] = FALSE;
        }
}
