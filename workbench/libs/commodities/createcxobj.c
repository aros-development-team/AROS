/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <libraries/commodities.h>
#include <proto/exec.h>
#include <proto/commodities.h>
#include <string.h>

#include <aros/debug.h>

VOID BrokerFunc(CxObj *, struct NewBroker *, struct Library *CxBase);


    AROS_LH3(CxObj *, CreateCxObj,

/*  SYNOPSIS */

	AROS_LHA(ULONG, type, D0),
	AROS_LHA(IPTR,  arg1, A0),
	AROS_LHA(IPTR,  arg2, A1),

/*  LOCATION */

	struct Library *, CxBase, 5, Commodities)

/*  FUNCTION

    Creates a commodity object of type 'type'. This function should never
    be called directly; instead, use the macros defined in <libraries/
    commodties.h>. Brokers, however, should be created with the CxBroker()
    function.

    INPUTS

    type  -  the type of the commodity object to be created. Possible
             types are defined in <libraries/commodities.h>.
    arg1  -  depends on the value of 'type' above.  
    arg2  -  depends on the value of 'type' above.

    RESULT

    The commodity object or NULL if it couldn't be created. Not so severe
    problems in the creation process are recorded in an internal field
    retrievable with CxObjError(). These errors are defined in <libraries/
    commodities.h>

    NOTES

    This 'CxObj *' that is returned from this function (and from CxBroker())
    is the reference to your commodity object. It shall be used whenever
    dealing with your commodity objects (functions operating on commodity
    objects and so on).

    EXAMPLE

    BUGS

    SEE ALSO

    CxObjError(), CxBroker(), cx_lib/CxSender(), cx_lib/CxSignal(),
    cx_lib/CxFilter(), cx_lib/CxTranslate() cx_lib/CxCustom(),
    cx_lib/CxDebug()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    CxObj *co;

    D(bug("CreateCxObject: Entering..., trying to create an object "
	  "of type %d\n", type));

    if ((co = (CxObj *)AllocCxStructure(CX_OBJECT, type, CxBase)) == NULL)
    {
	return NULL;      /* No memory for object */
    }

    D(bug("CreateCxObject: Memory for object allocated.\n"));

    co->co_Node.ln_Type = type;
    co->co_Flags = COF_ACTIVE;
    co->co_Error = 0;

    NEWLIST(&co->co_ObjList);

    switch (type)
    {
    case CX_FILTER:
	SetFilter(co, (STRPTR)arg1);
	break;
	
    case CX_TYPEFILTER:
	co->co_Ext.co_FilterIX = (IX *)arg1;
	break;
	
    case CX_SEND:
	co->co_Ext.co_SendExt->sext_MsgPort = (struct MsgPort *)arg1;
	co->co_Ext.co_SendExt->sext_ID = (ULONG)arg2;
	break;
	
    case CX_SIGNAL:
	co->co_Ext.co_SignalExt->sixt_Task = (struct Task *)arg1;
	co->co_Ext.co_SignalExt->sixt_SigBit = (UBYTE)arg2;
	break;
	
    case CX_TRANSLATE:
	co->co_Ext.co_IE = (struct InputEvent *)arg1;
	break;
	
    case CX_BROKER:
	BrokerFunc(co, (struct NewBroker *)arg1, CxBase);
	break;
	
    case CX_DEBUG:
	co->co_Ext.co_DebugID = arg1;
	break;
	
    case CX_CUSTOM:
	co->co_Ext.co_CustomExt->cext_Action = (APTR)arg1;
	co->co_Ext.co_CustomExt->cext_ID = arg2;
	break;
	
    default: 
	break;
    }
    
    D(bug("CreateCxObject: Leaving...\n"));
    
    return co;

    AROS_LIBFUNC_EXIT
} /* CxBroker */


VOID BrokerFunc(CxObj *co, struct NewBroker *nbrok, struct Library *CxBase)
{
    strncpy(co->co_Ext.co_BExt->bext_Name,  nbrok->nb_Name,  CBD_NAMELEN);
    strncpy(co->co_Ext.co_BExt->bext_Title, nbrok->nb_Title, CBD_TITLELEN);
    strncpy(co->co_Ext.co_BExt->bext_Descr, nbrok->nb_Descr, CBD_DESCRLEN);
    
    co->co_Ext.co_BExt->bext_Task = FindTask(NULL);
    
    /* Brokers are created inactive */
    co->co_Flags &= ~COF_ACTIVE;
    co->co_Node.ln_Pri = nbrok->nb_Pri;
    
    co->co_Ext.co_BExt->bext_MsgPort = nbrok->nb_Port;

    co->co_Flags |= (UBYTE)nbrok->nb_Flags;
}
