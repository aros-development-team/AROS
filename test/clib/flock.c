#include <sys/file.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include "test.h"

char *tmpname;
struct SignalSemaphore sem;

#define ITERATIONS 50
#define NPROCS 10

LONG entry()
{
    struct Task *this = FindTask(NULL);
    Wait(SIGBREAKF_CTRL_C);
    int *counter = this->tc_UserData;
    int i, fd;
    struct Library *DOSBase;
    
    DOSBase = OpenLibrary("dos.library", 0);
    if(!DOSBase)
        return -1;
    
    ObtainSemaphore(&sem);
    fd = open(tmpname, O_RDONLY);
    ReleaseSemaphore(&sem);

    for(i = 0; i < ITERATIONS; i++)
    {
	if(!flock(fd, LOCK_EX))
	{
	    int tmp = *counter;
	    Delay(1);
	    *counter = tmp + 1;
	    printf("\rprogress: %.2f%%", *counter * 100.0 / (NPROCS * ITERATIONS));
	    flock(fd, LOCK_UN);
	}
	else
	{
	    close(fd);
	    return -1;
	}
    }

    close(fd);
    
    CloseLibrary(DOSBase);
    return 0;
}

int main()
{
    tmpname = mktemp("T:flockXXXXXX");
    int fd = open(tmpname, O_CREAT);
    TEST((fd != -1));

    TEST((flock(fd, LOCK_SH|LOCK_NB) == 0));
    TEST((flock(fd, LOCK_UN) == 0));

    TEST((flock(fd, LOCK_EX|LOCK_NB) == 0));
    TEST((flock(fd, LOCK_UN) == 0));

    TEST((flock(fd, LOCK_SH) == 0));
    TEST((flock(fd, LOCK_UN) == 0));

    TEST((flock(fd, LOCK_EX) == 0));
    TEST((flock(fd, LOCK_UN) == 0));

    /* Create NPROCS processes increasing counter ITERATIONS times in an ugly 
       way */
    int counter = 0;
    struct Process *procs[NPROCS];
    ULONG ids[NPROCS];
    struct TagItem tags[] =
    {
	{ NP_Entry,         (IPTR) entry     },
        { NP_Name,          (IPTR) "flocker" },
        { NP_Output,        (IPTR) Output()  },
        { NP_CloseOutput,   (IPTR) FALSE     },
        { NP_UserData,      (IPTR) &counter  },
        { NP_NotifyOnDeath, (IPTR) TRUE      },
        { TAG_DONE,         0                }
    };

    int i;
    InitSemaphore(&sem);
    for(i = 0; i < NPROCS; i++)
    {
	procs[i] = CreateNewProc(tags);
	TEST((procs[i]));
	ids[i] = GetETask(procs[i])->et_UniqueID;
	Signal((struct Task *)procs[i], SIGBREAKF_CTRL_C);
    }
    
    for(i = 0; i < NPROCS; i++)
    {
	ChildWait(ids[i]);
	ChildFree(ids[i]);
    }
    putchar('\n');
    
    TEST((counter == NPROCS * ITERATIONS));
    
    cleanup();
    return OK;
}

void cleanup()
{
    remove(tmpname);
}
