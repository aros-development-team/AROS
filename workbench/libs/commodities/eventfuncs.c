/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Commodities initialization code.
    Lang: English.
*/


#ifndef COMMODITIES_BASE_H
#include "cxintern.h"
#endif
#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif
#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif
#ifndef DEVICES_INPUTEVENT_H
#include <devices/inputevent.h>
#endif


static ULONG Extensions[] =
{
    0,				        /* CX_INVALID */
    sizeof(struct InputXpression),	/* CX_FILTER */
    0,				        /* CX_TYPEFILTER */
    sizeof(struct SendExt),		/* CX_SENDER */
    sizeof(struct SignalExt),	        /* CX_SIGNAL */
    0,				        /* CX_TRANSLATE */
    sizeof(struct BrokerExt),	        /* CX_BROKER */
    0,				        /* CX_DEBUG */
    sizeof(struct CustomExt),       	/* CX_CUSTOM */
    0				        /* CX_ZERO */
};


VOID FreeCxStructure(APTR obj, int type, struct Library *CxBase)
{
    if (obj == NULL)
    {
	return;
    }
 
    switch (type)
    {
    case CX_OBJECT:
	FreeVec(obj);
	break;
	
    case CX_MESSAGE:
	FreeCxStructure(((CxMsg *)obj)->cxm_Data, CX_INPUTEVENT, CxBase);
	FreeVec(obj);
	break;
	
    case CX_INPUTEVENT:
	if(((struct InputEvent *)obj)->ie_Class == IECLASS_NEWPOINTERPOS &&
	   (((struct InputEvent *)obj)->ie_SubClass == IESUBCLASS_TABLET ||
	    ((struct InputEvent *)obj)->ie_SubClass == IESUBCLASS_NEWTABLET ||
	    ((struct InputEvent *)obj)->ie_SubClass == IESUBCLASS_PIXEL))
	{
	    FreeVec(((struct InputEvent *)obj)->ie_EventAddress);
	}

	FreeMem(obj, sizeof(struct GeneratedInputEvent));
	break;
    }
}


APTR AllocCxStructure(LONG type, LONG objtype, struct Library *CxBase)
{
    APTR   temp = NULL;
    CxObj *tempObj;
    CxMsg *tempMsg;
    
    switch (type)
    {
    case CX_OBJECT:
	tempObj = (CxObj *)AllocVec(sizeof(CxObj) + Extensions[objtype],
				    MEMF_CLEAR | MEMF_PUBLIC);

	tempObj->co_Ext.co_FilterIX = (APTR)(tempObj + 1);

	NEWLIST(&tempObj->co_ObjList);
	
	/* This is done to make it easy for Exchange */
	if (objtype == CX_BROKER)
	{
	    tempObj->co_Node.ln_Name = (char *)&tempObj->co_Ext.co_BExt->bext_Name;
	}

	temp = (APTR)tempObj;
	break;
	
    case CX_MESSAGE: 
	
	switch (objtype)
	{
	case CXM_SINGLE:
	    tempMsg = AllocVec(sizeof(CxMsg), MEMF_CLEAR | MEMF_PUBLIC);
	    
	    if (tempMsg != NULL)
	    {
		tempMsg->cxm_Data = NULL;
		tempMsg->cxm_Type = CXM_COMMAND;
		tempMsg->cxm_Message.mn_ReplyPort = &GPB(CxBase)->cx_MsgPort;
		tempMsg->cxm_Message.mn_Length = sizeof(CxMsg);
		temp = (APTR)tempMsg;
	    }

	    break;
	    
	case CXM_DOUBLE:
	    tempMsg = AllocVec(sizeof(CxMsg), MEMF_CLEAR | MEMF_PUBLIC);
	    
	    if (tempMsg != NULL)
	    {
		tempMsg->cxm_Type = CXM_IEVENT;
		tempMsg->cxm_Data = AllocCxStructure(CX_INPUTEVENT, 0, CxBase);
		tempMsg->cxm_Message.mn_ReplyPort = &GPB(CxBase)->cx_MsgPort;
		tempMsg->cxm_Message.mn_Length = sizeof(CxMsg);
		
		if (tempMsg->cxm_Data == NULL)
		{
		    FreeVec(tempMsg);
		    tempMsg = NULL;
		}
		
		temp = (APTR)tempMsg;
	    }

	    break;
	}

	break;
	
    case CX_INPUTEVENT:
	
	temp = AllocMem(sizeof(struct GeneratedInputEvent), MEMF_CLEAR | MEMF_PUBLIC);
	break;
    }

    return temp;
}

