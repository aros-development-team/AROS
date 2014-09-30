/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <aros/debug.h>
#include "test.h"

#define EXIT_STATUS 123

int main()
{
    char *argv0[] = { "Echo", "I'm child", NULL };
    char *envp[] = { NULL };
    pid_t pid;
    pid_t wait_pid;
    int status;

    pid = vfork();
    if((int) pid > 0)
    {
	printf("I'm parent, I have a child with pid %d\n", (int) pid);
	printf("Waiting for child to exit.\n");
	wait_pid = wait(&status);
	TEST((wait_pid == pid));
	printf("Child %d exited with exit status %d\n", (int) wait_pid, status);
	TEST((status == EXIT_STATUS));
    }
    else if(pid == 0)
    {
	printf("Exiting with status %d\n", EXIT_STATUS);
	_exit(EXIT_STATUS);
    }
    else
    {
	TEST(0);
    }

    pid = vfork();
    if((int) pid > 0)
    {
	printf("I'm parent, I have a child with pid %d\n", (int) pid);
	printf("Waiting for child to exit.\n");
	wait_pid = wait(&status);
	TEST((wait_pid == pid));
	printf("Child %d exited with exit status %d\n", (int) wait_pid, status);
	TEST((status == 0));
    }
    else if(pid == 0)
    {
	execve("C:Echo", argv0, envp);
	_exit(EXIT_STATUS);
    }
    else
    {
	TEST(0);
    }
    return 0;
}

void cleanup()
{
}
