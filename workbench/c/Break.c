/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Break - send a signal to a process.
    Lang: english
*/

/*****************************************************************************

    NAME
	Break

    FORMAT
	Break <process> [ALL|C|D|E|F]

    SYNOPSIS
	PROCESS/A/N,ALL/S,C/S,D/S,E/S,F/S

    LOCATION
	Workbench:c

    FUNCTION
	BREAK sends a signal to another process to get its attention.
	The argument |PROCESS| specifies the number of the process that
	you wish to send the signal to. You can find out process numbers
	with the STATUS command.

	You can send a combination of the flags CTRL-C, CTRL-D, CTRL-E
	and CTRL-F. By default, only the CTRL-C flag is sent.

	The effect of using the BREAK command is the same as selecting
	the console window of a process and pressing the relevant key
	combination.

	The normal meaning of the keys is:
	    CTRL-C	-	Halt a process
	    CTRL-D	-	Halt a shell script
	    CTRL-E	-	Close a process' window
	    CTRL-F	-	Make active the process' window

	Not all programs respond to these signals, however most should
	respond to CTRL-C.

    EXAMPLE
	
	1.SYS:> BREAK 1

	    Send the CTRL-C signal to the process numbered 1.

	1.SYS:> BREAK 4 E

	    Send the CTRL-E signal to the process numbered 4.

    SEE ALSO

	STATUS

**************************************************************************/

#include <exec/types.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARG_TEMPLATE	"PROCESS/A/N,C/S,D/S,E/S,F/S,ALL/S"
#define ARG_PROCESS	0
#define ARG_C		1
#define ARG_D		2
#define ARG_E		3
#define ARG_F		4
#define ARG_ALL		5
#define TOTAL_ARGS	6

static const char version[] = "$VER: Break 41.1 (3.1.1998)";
static const char exthelp[] =
    "Break : Set the attention flags of a DOS task\n"
    "\tPROCESS/A/N  Process to set flags for\n"
    "\tALL/S        Set ALL attention flags\n"
    "\tC/S          Set the CTRL-C flag\n"
    "\tD/S          Set the CTRL-D flag\n"
    "\tE/S          Set the CTRL-E flag\n"
    "\tF/S          Set the CTRL-F flag\n";

int __nocommandline = 1;

int main(void)
{
    struct Process *pr = NULL;
    struct RDArgs *rd, *rda = NULL;
    IPTR args[TOTAL_ARGS] = { 0, TRUE, FALSE, FALSE, FALSE, FALSE };
    BPTR errStream = Output();
    ULONG mask = 0;
    int error = 0;

    if((rda = AllocDosObject(DOS_RDARGS, NULL)))
    {
	rda->RDA_ExtHelp = (STRPTR)exthelp;

	if((rd = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL)))
	{
	    pr = FindCliProc(*(IPTR *)args[ARG_PROCESS]);
	    if( pr != NULL )
	    {
		/* Figure out the mask of flags to send. */
		if( args[ARG_ALL] )
		    mask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D 
			 | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F;
		else
		    mask =  (args[ARG_C] == TRUE ? SIGBREAKF_CTRL_C : 0)
			  |	(args[ARG_D] == TRUE ? SIGBREAKF_CTRL_D : 0)	
			  |	(args[ARG_E] == TRUE ? SIGBREAKF_CTRL_E : 0)
			  |	(args[ARG_F] == TRUE ? SIGBREAKF_CTRL_F : 0);

		Signal((struct Task *)pr, mask);
	    }
	    else
	    {
		/* There is no relevant error code, OBJECT_NOT_FOUND
		    is a filesystem error, so we can't use that... */

		pr = (struct Process *)FindTask(NULL);
		if (pr->pr_CES != NULL)
		    errStream = pr->pr_CES;

		VFPrintf(errStream, "Break: Process not found.\n", NULL);
		error = -1;
	    }

	    FreeArgs(rd);
	} /* ReadArgs() ok */
	else
	    error = IoErr();

	FreeDosObject(DOS_RDARGS, rda);
    } /* Got rda */
    else
	error = IoErr();

    if( error != 0 && error != -1)
    {
	PrintFault(error, "Break");
	return RETURN_FAIL;
    }

    SetIoErr(0);
    return 0;
}	    
