/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include <aros/debug.h>
#include <proto/exec.h>
#include <devices/inputevent.h>
#include <exec/memory.h>
#include "cxintern.h"

#define DEBUG_COPYIEVENT(x)	x;

BOOL CopyInputEvent(struct InputEvent *from, struct InputEvent *to, struct CommoditiesBase *CxBase);

    AROS_LH1(VOID, AddIEvents,

/*  SYNOPSIS */

	AROS_LHA(struct InputEvent *, events, A0),

/*  LOCATION */

	struct Library *, CxBase, 30, Commodities)

/*  FUNCTION

    Send input events though the commodity hierarchy. After the call the
    list of InputEvents may be disposed.

    INPUTS

    events  --  a NULL-terminated linked list of InputEvents (may be NULL).

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    CxMsg *msg;

    if (events == NULL)
    {
	return;
    }
    
    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    do
    {
	msg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE, CxBase);
	
	if (msg == NULL)
	{
	    break;
	}
	
	if (!CopyInputEvent(events, msg->cxm_Data, GPB(CxBase)))
	{
	    DEBUG_COPYIEVENT(dprintf("AddIEvents: CopyInputEvent() failed!\n"));
	}
	
	ROUTECxMsg(msg, (CxObj *) GPB(CxBase)->cx_BrokerList.lh_Head);
	AddTail(&GPB(CxBase)->cx_MessageList, (struct Node *)msg);
	
	events = events->ie_NextEvent;
    } while(events != NULL);
    
    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    AROS_LIBFUNC_EXIT
} /* AddIEvents */
