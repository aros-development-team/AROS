/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#define  AROS_ALMOST_COMPATIBLE

#ifndef  DEBUG
#define  DEBUG 0
#endif

#include "cxintern.h"

#include <aros/debug.h>

#include <devices/input.h>
#include <proto/commodities.h>
#include <proto/exec.h>
#include <devices/inputevent.h>
#include <libraries/commodities.h>
#include <exec/lists.h>
#include <aros/asmcall.h>

#ifdef __MORPHOS__
extern const struct EmulLibEntry cxIHandler_Gate;
#else
extern struct InputEvent *cxIHandler();
#endif


    AROS_LH2(CxObj *, CxBroker,

/*  SYNOPSIS */

	AROS_LHA(struct NewBroker *, nb   , A0),
	AROS_LHA(LONG *            , error, D0),

/*  LOCATION */

	struct Library *, CxBase, 6, Commodities)

/*  FUNCTION

    Create a commodity broker from the specifications found in the structure
    pointed to by 'nb'. The NewBroker structure is described in <Libraries/
    Commodities.h>, see this file for more info. After the call, the 
    NewBroker structure isn't needed anymore and may be discarded.

    INPUTS

    nb    --  pointer to an initialized NewBroker structure
    error --  pointer to a LONG where the possible error of the CxBroker
              function is stored (may be NULL)

    RESULT

    A pointer to a commodity broker, or NULL upon failure.  If 'error' is
    NULL, no error information is stored. The possible error types are

    CBERR_OK       --  everything went just fine

    CBERR_SYSERR   --  system problems, typically not enough memory

    CBERR_DUP      --  another broker with the same name already exists
                       (and your nb_Unique indicates that only one is
		       allowed)

    CBERR_VERSION  --  the version found in nb_Version is unknown to the
                       library

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    SetCxObjPri(), <libraries/commodities.h>

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    LONG    myerr = CBERR_OK;
    CxObj  *co = NULL;
    CxObj  *temp;

    D(bug("Entering CxBroker\n"));
    
    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    /* No duplicates allowed? */
    if (nb->nb_Unique & NBU_UNIQUE)
    {
	temp = (CxObj *)FindName(&GPB(CxBase)->cx_BrokerList, nb->nb_Name);
	
	if (temp != NULL)
	{
	    if(nb->nb_Unique & NBU_NOTIFY)
	    {
		CheckStatus(temp, CXCMD_UNIQUE, CxBase);
	    }
	    
	    myerr = CBERR_DUP;
	}
    }
    
    if (myerr == CBERR_OK)
    {
	if ((co = CreateCxObj(CX_BROKER, (IPTR)nb, (IPTR)NULL)) != NULL)
	{
	    if (co->co_Ext.co_BExt->bext_MsgPort != NULL)
	    {
		if (!GPB(CxBase)->cx_Running)
		{
		    if (SetupIHandler((struct CommoditiesBase *)CxBase) == FALSE)
		    {
			goto sysErr;
		    }
		    else
		    {
			GPB(CxBase)->cx_Running = TRUE;
		    }
		}
		else
		{
		    CxNotify(NULL, CXCMD_LIST_CHG);
		}
		
		Enqueue(&GPB(CxBase)->cx_BrokerList, (struct Node *)co);
		co->co_Flags |= COF_VALID;
	    }
	    else
	    {
		myerr = CBERR_VERSION;
	    }
	}
	else
	{
	sysErr:
	    myerr = CBERR_SYSERR;
	}
    }
    
    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    if (error != NULL)
    {
	*error = myerr;
    }

    D(bug("CxBroker: returning co=%p\n", co));
    
    return co;
    
    AROS_LIBFUNC_EXIT
} /* CxBroker */


BOOL SetupIHandler(struct CommoditiesBase *CxBase)
{
    D(bug("CxBroker: Setting up input handler.\n"));
    
    CxBase->cx_InputMP.mp_Node.ln_Type = NT_MSGPORT;
    CxBase->cx_InputMP.mp_Flags = PA_SIGNAL;
    CxBase->cx_InputMP.mp_SigBit = SIGB_SINGLE;
    CxBase->cx_InputMP.mp_SigTask = FindTask(NULL);
    NEWLIST(&CxBase->cx_InputMP.mp_MsgList);
    CxBase->cx_IORequest.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    CxBase->cx_IORequest.io_Message.mn_Length = sizeof(struct IOStdReq);
    CxBase->cx_IORequest.io_Message.mn_ReplyPort = &CxBase->cx_InputMP;
    
    if (OpenDevice("input.device", 0,
		   (struct IORequest *)&CxBase->cx_IORequest, 0) != 0)
    {
	// kprintf("Input.device didn't open\n");
	return FALSE;
    }
    
    //  kprintf("CxBroker: Opened input.device.\n");
    
    CxBase->cx_Interrupt.is_Code = (VOID (*)())AROS_ASMSYMNAME(cxIHandler);
    CxBase->cx_Interrupt.is_Data = CxBase;
    CxBase->cx_Interrupt.is_Node.ln_Pri = 53;
    CxBase->cx_Interrupt.is_Node.ln_Name = CxBase->cx_Lib.lib_Node.ln_Name;
    CxBase->cx_IORequest.io_Command = IND_ADDHANDLER;
    CxBase->cx_IORequest.io_Data = &CxBase->cx_Interrupt;
    
    DoIO((struct IORequest *)&CxBase->cx_IORequest);
    
    // kprintf("CxBroker: Handler up and running.\n");
    
    return TRUE;
}
