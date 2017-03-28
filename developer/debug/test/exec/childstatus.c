/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <assert.h>

LONG entry()
{
    Delay(50);
    return 0;
}

int main()
{
    struct Process *child;
    
    struct TagItem tags[] =
    {
	{ NP_Entry,         (IPTR) entry              },
    	{ NP_Cli,           (IPTR) TRUE               },
        { NP_Name,          (IPTR) "test"             },
        { NP_NotifyOnDeath, (IPTR) TRUE               },
        { TAG_DONE,         0                         }
    };
    
    child = CreateNewProc(tags);
    
    if(child)
    {
	ULONG childstatus;
	ULONG childid = GetETask((struct Task*) child)->et_UniqueID;
	Printf("Checking status value for non-existing child id\n");
	childstatus = ChildStatus(-1);
	assert(childstatus == CHILD_NOTFOUND);
	Printf("Result: CHILD_NOTFOUND\n");
	Printf("Checking status value for running child id\n");
	childstatus = ChildStatus(childid);
	assert(childstatus == CHILD_ACTIVE);
	Printf("Result: CHILD_ACTIVE\n");
	ChildWait(childid);
	Printf("Checking status value for died child id\n");
	childstatus = ChildStatus(childid);
	assert(childstatus == CHILD_EXITED);
	Printf("Result: CHILD_EXITED\n");
	ChildFree(childid);
	Printf("Checking status value for freed child id\n");
	childstatus = ChildStatus(childid);
	assert(childstatus == CHILD_NOTFOUND);
	Printf("Result: CHILD_NOTFOUND\n");
    }
    else
	PrintFault(IoErr(), "Couldn't create child process");
    return 0;
}
