/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Commodities initialization code.
    Lang: English.
*/

#define  USE_ZERO_OBJECT 0 /* stegerg: no idea why zero object is/was used at all */

#ifndef  DEBUG
#define  DEBUG 0
#endif

#include <utility/utility.h>
#include "cxintern.h"	/* Must be included after utility.h */ 

#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <devices/timer.h>

#include <intuition/classusr.h>
#include <exec/libraries.h>
#include <exec/alerts.h>
#include <libraries/commodities.h>
#include <proto/commodities.h>
#include LC_LIBDEFS_FILE

BOOL InitCx(struct CommoditiesBase *CxBase);
VOID ShutDownCx(struct CommoditiesBase *CxBase);

AROS_SET_LIBFUNC(Init, struct CommoditiesBase, CxBase)
{
    AROS_SET_LIBFUNC_INIT
    
    BOOL ok = TRUE;
    
    /*
      This function is single-threaded by exec by calling Forbid.
      If you break the Forbid() another task may enter this function
      at the same time. Take care.
    */
    
    D(bug("commodities_open: Entering...\n"));
    
    CxBase->cx_TimerMP.mp_Node.ln_Type = NT_MSGPORT;
    CxBase->cx_TimerMP.mp_Flags = PA_IGNORE;
    NEWLIST(&CxBase->cx_TimerMP.mp_MsgList);
	
    CxBase->cx_TimerIO.tr_node.io_Message.mn_ReplyPort = &CxBase->cx_TimerMP;
    CxBase->cx_TimerIO.tr_node.io_Message.mn_Length = sizeof(struct timerequest);
	
    if (OpenDevice(TIMERNAME, UNIT_VBLANK,
		   (struct IORequest *)&CxBase->cx_TimerIO, 0) == 0)
    {
	CxBase->cx_TimerBase = 
	    (struct Library *)(CxBase->cx_TimerIO.tr_node.io_Device);
    }
    if (CxBase->cx_TimerBase == NULL)
	return FALSE;

    D(bug("commodities_open: Setting up Zero object.\n"));
	
    ok = InitCx((struct CommoditiesBase *)CxBase);
    
    if (!ok)
    {
        D(bug("Error: Failed to initialize commodities.library.\n"));
	ShutDownCx((struct CommoditiesBase *)CxBase);

	return FALSE;
    }
    
    D(bug("commodities_open: Library correctly opened.\n"));
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}


BOOL InitCx(struct CommoditiesBase *CxBase)
{
#if USE_ZERO_OBJECT
    CxObj *zero;
#endif

    InitSemaphore(&CxBase->cx_SignalSemaphore);
    NEWLIST(&CxBase->cx_BrokerList);
    NEWLIST(&CxBase->cx_MessageList);
    NEWLIST(&CxBase->cx_GeneratedInputEvents);
    
    CxBase->cx_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
    CxBase->cx_MsgPort.mp_Flags = PA_IGNORE;
    NEWLIST(&CxBase->cx_MsgPort.mp_MsgList);
    
#if USE_ZERO_OBJECT
    zero = CreateCxObj(CX_ZERO, (IPTR)NULL, (IPTR)NULL);
    
    if (zero == NULL)
    {
	return FALSE;
    }

    /* Make sure this object goes LAST in the list */
    ((struct Node *)zero)->ln_Pri = -128;

    zero->co_Flags |= COF_VALID;
    AddHead(&CxBase->cx_BrokerList, (struct Node *)zero);
#endif

    return TRUE;
}


VOID ShutDownCx(struct CommoditiesBase *CxBase)
{
    struct InputEvent *temp;
    CxMsg *msg;
 
    /* Free messages */
    while ((msg = (CxMsg *)GetMsg(&CxBase->cx_MsgPort)) != NULL)
    {
	FreeCxStructure(msg, CX_MESSAGE, (struct Library *)CxBase);
    }
    
    /* Free input events */
    while (CxBase->cx_IEvents != NULL)
    {
	temp = CxBase->cx_IEvents->ie_NextEvent;
	FreeCxStructure(CxBase->cx_IEvents, CX_INPUTEVENT,
			(struct Library *)CxBase);
	CxBase->cx_IEvents = temp;
    }

    CxBase->cx_IEvents = NULL;
    
#if USE_ZERO_OBJECT    
    /* Remove the ZERO object, in case it exists. */
    DeleteCxObj((CxObj *)RemHead(&CxBase->cx_BrokerList));
#endif
}


AROS_SET_LIBFUNC(Expunge, struct CommoditiesBase, CxBase)
{
    AROS_SET_LIBFUNC_INIT
    
    ShutDownCx(CxBase);
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
