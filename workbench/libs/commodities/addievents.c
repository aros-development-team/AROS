
/*
    (C) 1997-99 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <proto/exec.h>
#include <devices/inputevent.h>
#include <exec/memory.h>

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
	
	*(msg->cxm_Data) = *events;
	
	/* Copy the structure pointed to by ie_EventAddress in case of
	   a NEWPOINTERPOS event */
	
	if (events->ie_Class == IECLASS_NEWPOINTERPOS)
	{
	    switch (events->ie_SubClass)
	    {
	    case IESUBCLASS_PIXEL :
		msg->cxm_Data->ie_EventAddress =
		    AllocVec(sizeof(struct IEPointerPixel), MEMF_ANY);
		*((struct IEPointerPixel *)(msg->cxm_Data->ie_EventAddress)) =
		    *((struct IEPointerPixel *)(events->ie_EventAddress));
		break;
		
	    case IESUBCLASS_TABLET :
		msg->cxm_Data->ie_EventAddress =
		    AllocVec(sizeof(struct IEPointerTablet), MEMF_ANY);
		*((struct IEPointerTablet *)(msg->cxm_Data->ie_EventAddress)) =
		    *((struct IEPointerTablet *)(events->ie_EventAddress));
		break;
		
	    case IESUBCLASS_NEWTABLET :
		msg->cxm_Data->ie_EventAddress =
		    AllocVec(sizeof(struct IENewTablet), MEMF_ANY);
		*((struct IENewTablet *)(msg->cxm_Data->ie_EventAddress)) =
		    *((struct IENewTablet *)(events->ie_EventAddress));
		break;
		
	    default :
		break;
	    }
	}
	
	ROUTECxMsg(msg, (CxObj *) &GPB(CxBase)->cx_BrokerList.lh_Head);
	AddTail(&GPB(CxBase)->cx_MessageList, (struct Node *)msg);
	
	events = events->ie_NextEvent;
    } while(events != NULL);
    
    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    AROS_LIBFUNC_EXIT
} /* AddIEvents */
