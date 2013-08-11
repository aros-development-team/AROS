#include <unistd.h>
#include <stdio.h>
#include <aros/debug.h>
#include <sys/wait.h>
#include "test.h"

int main()
{
    pid_t pid;
    int retval = RETURN_OK;

    pid = vfork();
    if((int) pid > 0)
    {
        int status = -1;
	printf("I'm parent, I have a child with pid %d\n", (int) pid);
	waitpid(pid, &status, 0);
	if (status != 1)
	    return RETURN_FAIL;
    }
    else if(pid == 0)
    {
        printf("I'm child\n");
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
	pid_t pid2 = vfork();
	if((int) pid2 > 0)
	{
	    int status = -1;
	    printf("I'm parent, I have a second child with pid %d\n", (int) pid2);
	    waitpid(pid2, &status, 0);
	    if (status != 2)
	        return RETURN_FAIL;
	}
	else if(pid2 == 0)
	{
            printf("I'm second child\n");
	    _exit(2);
	}
	waitpid(pid, NULL, 0);
    }
    else if(pid == 0)
    {
	pid_t pid2 = vfork();
	int retval = 3;
	if((int) pid2 > 0)
	{
            printf("I'm child, I have my child with pid2 %d\n", (int) pid2);
	    int status = -1;
	    waitpid(pid2, &status, 0);
	    if (status != 4)
	        retval = RETURN_FAIL;
	}
	else if(pid2 == 0)
	{
	    printf("I'm the child of a child\n");
	    _exit(4);
	}
	_exit(retval);
    }
    else
    {
	TEST(0);
    }

    return retval;
}

void cleanup()
{
}
