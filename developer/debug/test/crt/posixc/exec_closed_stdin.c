/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A direct self-exec must preserve intentionally closed stdin.
*/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <proto/dos.h>
#include "test.h"

void cleanup(void)
{
}

int main(int argc, char **argv)
{
    char program[1024];
    char *const arguments[] = {program, "--child", NULL};
    char *const environment[] = {NULL};
    pid_t child, waited = 0;
    int status = -1, original_flags, result, error;
    unsigned int polls;

    if (argc == 2 && strcmp(argv[1], "--child") == 0)
    {
        errno = 0;
        result = fcntl(STDIN_FILENO, F_GETFD);
        error = errno;
        if (result != -1 || error != EBADF) return 21;
        if (!stdin || fileno(stdin) != STDIN_FILENO) return 22;
        errno = 0;
        result = fgetc(stdin);
        error = errno;
        if (result != EOF || error != EBADF) return 23;
        return 0;
    }

    original_flags = fcntl(STDIN_FILENO, F_GETFD);
    TEST(original_flags >= 0);
    TEST(NameFromLock(GetProgramDir(), (STRPTR)program, sizeof(program)) != 0);
    TEST(AddPart((STRPTR)program, FilePart((CONST_STRPTR)argv[0]), sizeof(program)) != 0);
    /* Arguments and environment are prepared before vfork; the child only
       closes its copied descriptor and execs, or exits on setup failure. */
    child = vfork();
    if (child == 0)
    {
        if (close(STDIN_FILENO) != 0) _exit(24);
        execve(program, arguments, environment);
        _exit(25);
    }
    TEST(child > 0);
    for (polls = 0; polls < 250; ++polls)
    {
        errno = 0;
        waited = waitpid(child, &status, WNOHANG);
        error = errno;
        if (waited != 0) break;
        Delay(1);
    }
    printf("child=%ld waited=%ld errno=%d status=%d polls=%u\n",
        (long)child, (long)waited, error, status, polls);
    TEST(waited == child);
    TEST(WIFEXITED(status));
    TEST(WEXITSTATUS(status) == 0);
    TEST(fcntl(STDIN_FILENO, F_GETFD) == original_flags);
    return OK;
}
