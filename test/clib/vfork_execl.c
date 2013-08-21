#include <unistd.h>
#include <stdio.h>
#include <aros/debug.h>
#include <sys/wait.h>
#include "test.h"

int main()
{
    pid_t pid;

    /* Test vfork and a child doing execl() */
    pid = vfork();
    if((int) pid > 0)
    {
        printf("I'm parent, I have a child with pid %d\n", (int) pid);
        waitpid(pid, NULL, 0);
    }
    else if(pid == 0)
    {
        execl("C:Echo", "Echo", "I'm child", NULL);
        _exit(1);
    }
    else
    {
        TEST(0);
    }
    printf("\n");

    /* Testing child trying to exec non-existing program */
    pid = vfork();
    if((int) pid > 0)
    {
        printf("I'm parent, I have a child with pid %d\n", (int) pid);
        waitpid(pid, NULL, 0);
    }
    else if(pid == 0)
    {
        TEST(execl(":XYZ/NotExist", "NotExist", "I'm child", NULL) == -1);
        _exit(0);
    }
    else
    {
        TEST(0);
    }

    /* Testing nested vfork() + execl() */
    pid = vfork();
    if((int) pid > 0)
    {
        printf("I'm parent, I have a first child with pid %d\n", (int) pid);
        pid_t pid2 = vfork();
        if((int) pid2 > 0)
        {
            printf("I'm parent, I have a second child with pid %d\n", (int) pid2);
            waitpid(pid2, NULL, 0);
        }
        else if(pid2 == 0)
        {
            execl("C:Echo", "Echo", "I'm the second child of a parent", NULL);
            _exit(1);
        }
        waitpid(pid, NULL, 0);
    }
    else if(pid == 0)
    {
        //printf("I'm child of a parent\n");
        pid_t pid2 = vfork();
        if((int) pid2 > 0)
        {
            //printf("I'm child, I have my child with pid %d\n", (int) pid2);
            waitpid(pid2, NULL, 0);
        }
        else if(pid2 == 0)
        {
            execl("C:Echo", "Echo", "I'm child of a child", NULL);
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
