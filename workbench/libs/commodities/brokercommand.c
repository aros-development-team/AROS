/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#ifndef  DEBUG
#define  DEBUG 0
#endif

#include "cxintern.h"

#include <aros/debug.h>

#include <dos/dos.h>
#include <proto/exec.h>
#include <libraries/commodities.h>

    AROS_LH2(ULONG, BrokerCommand,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, name   , A0),
	AROS_LHA(ULONG , command, D0),

/*  LOCATION */

	struct Library *, CxBase, 33, Commodities)

/*  FUNCTION

    Notify a task connected to a certain broker of a state change.

    INPUTS

    name     --  The name of the broker
    command  --  What to tell the task

    RESULT

    0 if everything was OK, a negative value otherwise:
    -1    --   Unknown broker 'name'
    -2    --   No broker message port
    -3    --   No memory for operation

    NOTES

    This function is present in AmigaOS too but undocumented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT
	
    static char Exg[] = "Exchange";
    
    ULONG error;
    CxObj *co;
    
    if (name == NULL)
    {
	name = Exg;
    }
    
    D(bug("Notifying %s\n", name));
    
    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    co = (CxObj *)FindName(&GPB(CxBase)->cx_BrokerList, name);
    error = CheckStatus(co, command, CxBase);
    
    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    D(bug("Notification done!\n"));
    
    return error;

    AROS_LIBFUNC_EXIT
} /* BrokerCommand */


ULONG CheckStatus(CxObj *broker, ULONG command, struct Library *CxBase)
{
    CxMsg *msg;
    
    if (broker == NULL)
    {
	return -1;
    }
    
    if (broker->co_Ext.co_BExt->bext_MsgPort == NULL)
    {
	if (command == CXCMD_KILL && broker->co_Ext.co_BExt->bext_Task != NULL)
	{
	    /* Tell the task to shut itself down */
	    Signal(broker->co_Ext.co_BExt->bext_Task, SIGBREAKF_CTRL_E);

	    return 0;
	}
	
	return -2;
    }
    
    msg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_SINGLE, CxBase);
    
    if (msg == NULL)
    {
	return -3;
    }
    
    msg->cxm_ID = command;
    PutMsg(broker->co_Ext.co_BExt->bext_MsgPort, (struct Message *)msg);
    
    return 0;
}
