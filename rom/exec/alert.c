/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1996/11/08 11:27:54  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.7  1996/10/24 15:50:43  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/23 14:24:23  aros
    Make sure the Alert is shown to the user

    Revision 1.5  1996/08/16 14:04:40  digulla
    Show more infos about the task

    Revision 1.4  1996/08/13 13:55:57  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:04  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <stdio.h>
#include <stdlib.h>

/*****************************************************************************

    NAME */
	#include <exec/alerts.h>
	#include <clib/exec_protos.h>

	AROS_LH1(void, Alert,

/*  SYNOPSIS */
	AROS_LHA(ULONG, alertNum, D7),

/*  LOCATION */
	struct ExecBase *, SysBase, 18, Exec)

/*  FUNCTION
	Alerts the user of a serious system problem.

    INPUTS
	alertNum - This is a number which contains information about
		the reason for the call.

    RESULT
	This routine may return, if the alert is not a dead-end one.

    NOTES
	You should not call this routine because it halts the machine,
	displays the message and then may reboot it.

    EXAMPLE
	// Dead-End alert: 680x0 Access To Odd Address
	Alert (0x80000003);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Task * task;

    task = FindTask (NULL);

    /* since this is an emulation, we just show the bug in the console */
    fprintf (stderr
	, "GURU Meditation %04lx %04lx %s\nTask: %p (%s)\n"
	, alertNum >> 16
	, alertNum & 0xFFFF
	, (alertNum & 0x80000000) ? "(DEADEND)" : ""
	, task, task->tc_Node.ln_Name
	);
    fflush (stderr);

    if (alertNum & AT_DeadEnd)
	exit (20);

    AROS_LIBFUNC_EXIT
} /* Alert */

