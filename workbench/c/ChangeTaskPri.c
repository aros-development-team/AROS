/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the priority of a task.
    Lang: english
*/

/**************************************************************************

    NAME
	ChangeTaskPri

    FORMAT
	ChangeTaskPri <priority> [ PROCESS <process number> ]

    SYNOPSIS
	PRI=PRIORITY/A/N,PROCESS/K/N

    LOCATION
	Workbench:c

    FUNCTION
	The ChangeTaskPri command is used to change the current run 
	priority of a Task. As AROS is a multitasking operating
	system, you can determine which tasks receive more CPU time
	by changing their priorities.

	The value of |priority| can be from -128 to 127, however values
	greater than 4 are not recommended as they can interfere with
	vital system processes. Higher values will give tasks a higher
	CPU priority.

	You can use the Status command to examine the list of Tasks that
	are running and their process numbers.

    EXAMPLE
	
	1.SYS:> ChangeTaskPri 1 Process 1

	    Set the priority of the current process to 1.

	1.SYS:> ChangeTaskPri 1

	    Also sets the priority of the current process to 1.

    SEE ALSO
	Status

**************************************************************************/

#include <exec/types.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#define ARG_TEMPLATE	"PRI=PRIORITY/A/N,PROCESS/K/N"
#define ARG_PRI		0
#define ARG_PROCESS	1
#define TOTAL_ARGS	2

static const char version[] = "$VER: ChangeTaskPri 41.2 (13.09.2005)";
static const char exthelp[] =
    "ChangeTaskPri : Change the priority of a CLI task\n"
    "\tPRI=PRIORITY/A/N      New priority of task\n"
    "\tPROCESS/K             Optional process number of change\n";

int __nocommandline = 1;

int main(void)
{
    struct Process *pr = NULL;
    struct RDArgs *rda = NULL, *rdargs;
    IPTR args[TOTAL_ARGS] = { 0, 0 };
    int error = 0;

    rda = AllocDosObject(DOS_RDARGS, NULL);
    if( rda != NULL )
    {
	rda->RDA_ExtHelp = (STRPTR)exthelp;

	rdargs = ReadArgs(ARG_TEMPLATE, (IPTR *)args, rda);
	if( rdargs != NULL )
	{
	    Forbid();
	    if( args[ARG_PROCESS] != 0 )
    		pr = FindCliProc(args[ARG_PROCESS]);
	    else
		pr = (struct Process *)FindTask(NULL);

	    if( pr != NULL )
	    {
	    	LONG pri = (LONG)(*(IPTR *)args[ARG_PRI]);
		
		/* Check the bounds on the priority */
		if(pri < -128 || pri > 127 )
		    error = ERROR_OBJECT_TOO_LARGE;
		else
    		    /* Set the priority */
    		    SetTaskPri( (struct Task *)pr, pri);
		Permit();
	    }
	    else
	    {
		BPTR errStream;
		Permit();
		errStream = Output();

		pr = (struct Process *)FindTask(NULL);
		if( pr->pr_CES != NULL )
		    errStream = pr->pr_CES;

		FPuts(errStream, "ChangeTaskPri: Process does not exist.\n");
		SetIoErr(0);
		error = -1;
	    }
	    FreeArgs(rdargs);
	} /* ReadArgs() ok */

	FreeDosObject(DOS_RDARGS, rda);
    } /* Got a RDArgs * */

    if( error != -1 && error != 0 )
    {
	PrintFault(error, "ChangeTaskPri");
	return RETURN_FAIL;
    }
    SetIoErr(0);
    return 0;
}
