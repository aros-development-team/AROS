/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <stdio.h>
#include <stdlib.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include <proto/arossupport.h>

/*****************************************************************************

    NAME */

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
	05-04-2008  danielo modified for osx

******************************************************************************/
{
    AROS_LIBFUNC_INIT
  
  KRNWireImpl(Alert);

  CALLHOOKPKT(krnAlertImpl,0,TAGLIST(TAG_USER,alertNum,TAG_DONE));
  
    AROS_LIBFUNC_EXIT
} /* Alert */

