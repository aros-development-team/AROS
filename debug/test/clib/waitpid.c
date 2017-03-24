/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <aros/debug.h>
#include <errno.h>
#include "test.h"

#define EXIT_STATUS 123

int main()
{
    pid_t pid;
    pid_t wait_pid;
    int status;
    char *argv[] = { "Delay", "50", NULL };
    char *envp[] = { NULL };

    wait_pid = waitpid(666, NULL, 0);
    TEST(wait_pid == -1);
    TEST(errno == ECHILD);
    
    pid = vfork();
    if((int) pid > 0)
    {
	printf("Created child with pid %d\n", (int) pid);
	printf("Waiting for child with pid %d to exit.\n", (int) pid);
	wait_pid = waitpid(pid, &status, 0);
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
	printf("Created child with pid %d\n", (int) pid);
	printf("Waiting for any child to exit.\n");
	wait_pid = waitpid(-1, &status, 0);
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
	printf("Created child with pid %d\n", (int) pid);
	printf("Waiting for any child to exit without hang.\n");
	wait_pid = waitpid(-1, &status, WNOHANG);
	if(wait_pid == 0) 
	    wait_pid = waitpid(-1, &status, 0);
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
	printf("Created child with pid %d\n", (int) pid);
	printf("Waiting for any child to exit without hang.\n");
	wait_pid = waitpid(-1, &status, WNOHANG);
	TEST((wait_pid == 0));
	printf("Child didn't exit yet\n");
	wait_pid = waitpid(-1, &status, 0);
	TEST((wait_pid == pid));
	printf("Child %d exited with exit status %d\n", (int) wait_pid, status);
	TEST((status == 0));

    }
    else if(pid == 0)
    {
	execve("C:Delay", argv, envp);
	_exit(-1);
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
