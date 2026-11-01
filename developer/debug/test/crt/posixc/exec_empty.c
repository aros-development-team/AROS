/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    execve must preserve empty arguments, including a trailing one.
*/
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
    pid_t child, waited = 0;
    int status = -1;
    unsigned int polls;

    if (argc > 1 && strcmp(argv[1], "--child") == 0)
    {
        int i;
        if (argc == 7 && argv[2][0] == '\0' &&
            strcmp(argv[3], "a b") == 0 && argv[4][0] == '\0' &&
            strcmp(argv[5], "tail") == 0 && argv[6][0] == '\0')
            return 0;
        fprintf(stderr, "child argc=%d\n", argc);
        for (i = 0; i < argc; ++i)
            fprintf(stderr, "argv[%d]=<%s>\n", i, argv[i]);
        return 24;
    }
    TEST(NameFromLock(GetProgramDir(), (STRPTR)program, sizeof(program)) != 0);
    TEST(AddPart((STRPTR)program, FilePart((CONST_STRPTR)argv[0]), sizeof(program)) != 0);
    child = vfork();
    if (child == 0)
    {
        char *const arguments[] = {program, "--child", "", "a b", "", "tail", "", NULL};
        char *const environment[] = {NULL};
        execve(program, arguments, environment);
        _exit(25);
    }
    TEST(child > 0);
    for (polls = 0; polls < 250; ++polls)
    {
        waited = waitpid(child, &status, WNOHANG);
        if (waited != 0)
            break;
        Delay(1);
    }
    TEST(waited == child);
    TEST(status == 0);
    return OK;
}
