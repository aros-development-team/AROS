/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native descriptor duplication and close-on-exec regression.
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <proto/dos.h>
#include "../test.h"

static int endpoints[2] = {-1, -1};
static int duplicate = -1;
static int ordinary = -1;
static int plain = -1;

void cleanup(void)
{
    if (ordinary >= 0)
        close(ordinary);
    if (plain >= 0)
        close(plain);
    if (duplicate >= 0)
        close(duplicate);
    if (endpoints[0] >= 0)
        close(endpoints[0]);
    if (endpoints[1] >= 0)
        close(endpoints[1]);
}

static int inspect(int fd, int *error)
{
    int result;

    errno = 0;
    result = fcntl(fd, F_GETFD);
    *error = errno;
    return result;
}

static int check_child(int original, int copy)
{
    int flags, error;

    flags = inspect(original, &error);
    printf("CHILD original=%d flags=%d errno=%d\n", original, flags, error);
    if (flags < 0)
        return 21;

    flags = inspect(copy, &error);
    printf("CHILD duplicate=%d flags=%d errno=%d expected_EBADF=%d\n",
        copy, flags, error, EBADF);
    return flags == -1 && error == EBADF ? 0 : 22;
}

int main(int argc, char **argv)
{
    char program[1024], original_arg[32], copy_arg[32];
    int flags, result, error, status = -1;
    pid_t child, waited = 0;
    unsigned int polls;

    if (argc == 4 && strcmp(argv[1], "--child") == 0)
        return check_child(atoi(argv[2]), atoi(argv[3]));

    TEST(NameFromLock(GetProgramDir(), (STRPTR)program, sizeof(program)) != 0);
    TEST(AddPart((STRPTR)program, FilePart((CONST_STRPTR)argv[0]), sizeof(program)) != 0);
    TEST(pipe(endpoints) == 0);

    TEST(fcntl(endpoints[0], F_SETFD, FD_CLOEXEC) == 0);
    ordinary = dup(endpoints[0]);
    TEST(ordinary >= 0);
    TEST(fcntl(ordinary, F_GETFD) == 0);
    TEST(close(ordinary) == 0);
    ordinary = -1;
    TEST(fcntl(endpoints[0], F_SETFD, 0) == 0);

    errno = 0;
    result = fcntl(endpoints[0], F_DUPFD_CLOEXEC, -1);
    error = errno;
    TEST(result == -1 && error == EINVAL);
    errno = 0;
    result = fcntl(-1, F_DUPFD_CLOEXEC, 32);
    error = errno;
    TEST(result == -1 && error == EBADF);

    printf("CONSTANTS F_DUPFD=%d F_DUPFD_CLOEXEC=%d F_GETFD=%d FD_CLOEXEC=%d\n",
        F_DUPFD, F_DUPFD_CLOEXEC, F_GETFD, FD_CLOEXEC);
    errno = 0;
    plain = fcntl(endpoints[0], F_DUPFD, 32);
    error = errno;
    printf("DUPFD minimum=32 result=%d errno=%d\n", plain, error);
    TEST(plain >= 32);
    flags = inspect(plain, &error);
    printf("DUPFD flags=%d errno=%d\n", flags, error);
    TEST(flags >= 0 && !(flags & FD_CLOEXEC));
    TEST(close(plain) == 0);
    plain = -1;

    errno = 0;
    duplicate = fcntl(endpoints[0], F_DUPFD_CLOEXEC, 32);
    error = errno;
    printf("DUPFD_CLOEXEC minimum=32 result=%d errno=%d\n", duplicate, error);
    TEST(duplicate >= 32);
    flags = inspect(duplicate, &error);
    printf("DUPFD_CLOEXEC flags=%d errno=%d\n", flags, error);
    TEST(flags >= 0 && (flags & FD_CLOEXEC));
    flags = inspect(endpoints[0], &error);
    printf("ORIGINAL fd=%d flags=%d errno=%d\n", endpoints[0], flags, error);
    TEST(flags >= 0);

    snprintf(original_arg, sizeof(original_arg), "%d", endpoints[0]);
    snprintf(copy_arg, sizeof(copy_arg), "%d", duplicate);
    fflush(stdout);
    child = vfork();
    if (child == 0)
    {
        execl(program, program, "--child", original_arg, copy_arg, (char *)NULL);
        _exit(111);
    }
    TEST(child > 0);

    for (polls = 0; polls < 250; ++polls)
    {
        errno = 0;
        waited = waitpid(child, &status, WNOHANG);
        error = errno;
        if (waited != 0 && !(waited < 0 && error == EINTR))
            break;
        Delay(1);
    }
    printf("COLLECT expected=%ld got=%ld status=%d errno=%d polls=%u\n",
        (long)child, (long)waited, status, error, polls);
    TEST(waited == child);
    TEST(status == 0);
    cleanup();
    printf("FCNTL_DUPLICATE_RESULT=0\n");
    return OK;
}
