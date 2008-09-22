#include <unistd.h>
#include <stdio.h>
#include <aros/debug.h>
#include "test.h"

int main()
{
    char *argv0[] = { "Echo", "I'm child", NULL };
    char *argv1[] = { "Echo", "I'm the second child of a parent", NULL };
    char *argv2[] = { "Echo", "I'm child of a child", NULL };
    char *envp[] = { NULL };
    pid_t pid;

    pid = vfork();
    if((int) pid > 0)
    {
	printf("I'm parent, I have a child with pid %d\n", (int) pid);
	sleep(1);
    }
    else if(pid == 0)
    {
	execve("C:Echo", argv0, envp);
	_exit(1);
    }
    else
    {
	TEST(0);
    }
  
    pid = vfork();
    if((int) pid > 0)
    {
	printf("I'm parent, I have a first child with pid %d\n", (int) pid);
	sleep(1);
	pid_t pid2 = vfork();
	if((int) pid2 > 0)
	{
	    printf("I'm parent, I have a second child with pid %d\n", (int) pid2);
	    sleep(1);
	}
	else if(pid2 == 0)
	{
	    execve("C:Echo", argv1, envp);
	    _exit(1);
	}
	_exit(0);
    }
    else if(pid == 0)
    {
	printf("I'm child of a parent\n");
	pid_t pid2 = vfork();
	if((int) pid2 > 0)
	{
	    printf("I'm child, I have my child with pid %d\n", (int) pid2);
	    sleep(1);
	}
	else if(pid2 == 0)
	{
	    execve("C:Echo", argv2, envp);
	    _exit(1);
	}
	_exit(0);
    }
    else
    {
	TEST(0);
    }

    return OK;
}

void cleanup()
{
}