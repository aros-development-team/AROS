/*
    (C) 1997-99 AROS - The Amiga Research OS
    $Id$

    Desc: Commodities input handler
    Lang: English
*/

/*  INTERNALS

  * Currently, there is no protection against stupid users. If you, for
    instance, installs a broker and attach a custom object to it, and this
    custom object's function is to Route the messages to the broker, there
    will be a deadlock, as the message list will never get empty.
	I have no GOOD solution for this, and therefore nothing to prevent
    this type of deadlock is implemented. One way could be to have a counter
    (in every message) that says how many times a message has been routed
    and if this counter reaches MAGICAL_VALUE the message is Disposed of.
	The above should then perhaps be accompanied by a new COMMAND,
    CXCMD_CYCLE that tells (at least Exchange) that something isn't quite
    right.

  * CX_TYPEFILTER isn't documented anywhere (and it's obsolete) meaning
    that the action done in case of a typefilter is not known to be the
    correct one.


    HISTORY

    ??.08.97   SDuvan Implemented
    16.08.99   Working version
*/

/***************************************************************************/

#include "cxintern.h"
#include <proto/exec.h>
#include <proto/commodities.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <aros/asmcall.h>
#include <stddef.h>

#define  DEBUG 1
#include <aros/debug.h>

static void ProduceEvent(CxMsg *, struct CommoditiesBase *CxBase);
static void DebugFunc(CxMsg *, CxObj *, struct CommoditiesBase *CxBase);
static void TransFunc(CxMsg *, CxObj *, struct CommoditiesBase *CxBase);
static void SendFunc(CxMsg *, CxObj *, struct CommoditiesBase *CxBase);
static BOOL CopyInputEvent(struct InputEvent *from, struct InputEvent *to,
	    struct CommoditiesBase *CxBase);

AROS_UFH2(struct InputEvent *, CxTree,
    AROS_UFHA(struct InputEvent *     , events , A0),
    AROS_UFHA(struct CommoditiesBase *, CxBase , A6)
	 )

{
    CxObj *co;
    CxMsg *tempMsg, *msg;
    struct Node *node, *succ;

    if(events == NULL)
	return NULL;
    
    if(IsListEmpty(&CxBase->cx_BrokerList))
	return events;

    ObtainSemaphore(&CxBase->cx_SignalSemaphore);

    /* Take care of the processed input events */
    ForeachNodeSafe(&CxBase->cx_GeneratedInputEvents, node, succ)
    {
        struct GeneratedInputEvent *tempEvent;

        tempEvent = (struct GeneratedInputEvent *)(((UBYTE *)node) - offsetof(struct GeneratedInputEvent, node));
        FreeCxStructure(tempEvent, CX_INPUTEVENT, (struct Library *)CxBase);
    }

    CxBase->cx_IEvents = NULL;
    CxBase->cx_EventExtra = NULL;
    NEWLIST(&CxBase->cx_GeneratedInputEvents);
    
    /* Free all the replied messages */
    while((tempMsg = (CxMsg *)GetMsg(&CxBase->cx_MsgPort)) != NULL)
	FreeCxStructure(tempMsg, CX_MESSAGE, (struct Library *)CxBase);


    /* Route all messages to the first broker */

    tempMsg = (CxMsg *)CxBase->cx_MessageList.lh_Head;

    co = (CxObj *)GetHead(&CxBase->cx_BrokerList);
    while(tempMsg != NULL)
    {
	tempMsg->cxm_Routing = co;
	tempMsg = (CxMsg *)tempMsg->cxm_Message.mn_Node.ln_Succ;
    }

    AddIEvents(events); /* Add the new events (incapsulated in commodtity
			   messages) to the message list. */

/***************************************************************************/

    /* Process the new events */

    while((msg = GetHead(&CxBase->cx_MessageList)) != NULL)
    {
	//	kprintf("Getting message %p\n", msg);

	co = msg->cxm_Routing;

	if(co != NULL)
	{
	    if(co->co_Node.ln_Succ == NULL)
	    {
		if(msg->cxm_Level != 0)
		{
		    msg->cxm_Level--;
		    co = msg->cxm_retObj[msg->cxm_Level];
		    co = (CxObj *)co->co_Node.ln_Succ;
		}		

		/* Neither successor broker nor return object exists => done */
		else
		{
		    co = NULL;
		}
	    }
	    else

	    /* Route the message to the next broker */
	    {
		// kprintf("Routing to next broker %p co = %p\n", co->co_Node.ln_Succ, co);
		msg->cxm_Routing = (CxObj *)co->co_Node.ln_Succ;
	    }
	}

	/* If there are no more objects that shall process the event, we
	   link it in to the list of ready input events */

	if(co == NULL)
	{
	    ProduceEvent(msg, CxBase);
	    continue;
	}

	if(!(co->co_Flags & COF_ACTIVE))
	    continue;

	// kprintf("Active: %i\n", co->co_Node.ln_Type);

	switch(co->co_Node.ln_Type)
	{
	case CX_INVALID:
	    break;
	    
	case CX_FILTER:
	    if(msg->cxm_Type == CXM_IEVENT)
	    {
		// kprintf("Filtering...");
		if(MatchIX(msg->cxm_Data, co->co_Ext.co_FilterIX) != 0)
		{
		    DivertCxMsg(msg, co, co);
		    //  kprintf("matched!");
		}
	    }
	    break;
	    
	case CX_TYPEFILTER:
	    if((msg->cxm_Type & co->co_Ext.co_TypeFilter) != 0)
		DivertCxMsg(msg, co, co);
	    
	    break;
	    
	case CX_SEND:
	    SendFunc(msg, co, CxBase);
	    break;
	    
	case CX_SIGNAL:
	    Signal(co->co_Ext.co_SignalExt->sixt_Task,
		   1 << co->co_Ext.co_SignalExt->sixt_SigBit);
	    break;
	
	case CX_TRANSLATE:
	    TransFunc(msg, co, CxBase);
	    break;
	    
	case CX_BROKER:
	    // kprintf("Broker diverting message...\n");
	    DivertCxMsg(msg, co, co);
	    break;
	    
	case CX_DEBUG:
	    DebugFunc(msg, co, CxBase);
	    break;
	    
	case CX_CUSTOM:
	    msg->cxm_ID = co->co_Ext.co_CustomExt->cext_ID;
	    (co->co_Ext.co_CustomExt->cext_Action)(msg, co);
	    break;
	    
	case CX_ZERO:
	    ProduceEvent(msg, CxBase);
	    break;
	}
    }
    
    ReleaseSemaphore(&CxBase->cx_SignalSemaphore);

    return CxBase->cx_IEvents;
}

    
static void ProduceEvent(CxMsg *msg, struct CommoditiesBase *CxBase)
{
    struct GeneratedInputEvent *temp;
  
    if((temp = (struct GeneratedInputEvent *)AllocCxStructure(CX_INPUTEVENT, 0,
	       (struct Library *)CxBase)) != NULL)
    {
        CopyInputEvent(msg->cxm_Data, &temp->ie, CxBase);

	/* Put the input event last in the ready list and update bookkeeping */
	temp->ie.ie_NextEvent = NULL;

	if(CxBase->cx_IEvents != NULL)
	{
	    *(CxBase->cx_EventExtra) = &temp->ie;
	}
	else
	{
	    CxBase->cx_IEvents = &temp->ie;
	}
	CxBase->cx_EventExtra = &temp->ie.ie_NextEvent;
	
	AddTail((struct List *)&CxBase->cx_GeneratedInputEvents, (struct Node *)&temp->node);
    }

    DisposeCxMsg(msg);
}


static void SendFunc(CxMsg *msg, CxObj *co, struct CommoditiesBase *CxBase)
{
CxMsg  *tempMsg;
  
    if(co->co_Ext.co_SendExt->sext_MsgPort == NULL)
	return;

    tempMsg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE,
					(struct Library *)CxBase);

    if(tempMsg == NULL) 
	return;

    CopyMem(msg, tempMsg, sizeof(CxMsg));

    CopyInputEvent(msg->cxm_Data, tempMsg->cxm_Data, CxBase);

    tempMsg->cxm_ID = co->co_Ext.co_SendExt->sext_ID;
    
    PutMsg(co->co_Ext.co_SendExt->sext_MsgPort, (struct Message *)tempMsg);
}


static void TransFunc(CxMsg *msg, CxObj *co, struct CommoditiesBase *CxBase)
{
    struct  InputEvent *event;
    CxMsg              *msg2;

    if(co->co_Ext.co_IE != NULL)
    {
        event = co->co_Ext.co_IE;
	
	do {
	    struct InputEvent *saveIE;  /* To save the InputEvent pointer
					   from being destroyed by CopyMem() */
	    
	    if((msg2 = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE,
			        (struct Library *)CxBase)) == NULL)
		break;

	    saveIE = msg2->cxm_Data;
	    CopyMem(msg, msg2, sizeof(CxMsg));
	    msg2->cxm_Data = saveIE;
	    
	    /* Don't care about errors for now */
	    CopyInputEvent(event, msg2->cxm_Data, CxBase);
	    
	    AddHead(&CxBase->cx_MessageList, (struct Node *)msg2);
	    
	} while((event = event->ie_NextEvent) != NULL);
    }

    DisposeCxMsg(msg);
}


static void DebugFunc(CxMsg *msg, CxObj *co, struct CommoditiesBase *CxBase)
{
    kprintf("\n----\nDEBUG NODE: %lx, ID: %lx\n"
	    "\tCxMsg: %lx, type: %x, data %lx destination %lx\n",
	    co, co->co_Ext.co_DebugID, msg->cxm_Routing, msg->cxm_Data,
	    msg->cxm_Type);
    
    if(msg->cxm_Type != CXM_IEVENT)
	return;
    
    kprintf("dump IE: %lx\n"
	    "\tClass %lx"
	    "\tCode %lx"
	    "\tQualifier %lx"
	    "\nEventAddress %lx",
	    msg->cxm_Data, msg->cxm_Data->ie_Class, msg->cxm_Data->ie_Code,
	    msg->cxm_Data->ie_Qualifier, msg->cxm_Data->ie_EventAddress);
}


static BOOL CopyInputEvent(struct InputEvent *from, struct InputEvent *to,
			   struct CommoditiesBase *CxBase)
{
    *to = *from;
    
    if(from->ie_Class == IECLASS_NEWPOINTERPOS)
    {
	switch(from->ie_SubClass)
	{
	case IESUBCLASS_PIXEL :
	    if((to->ie_EventAddress = AllocVec(sizeof(struct IEPointerPixel),
					       MEMF_ANY)) == NULL)
		return FALSE;
	    
	    *((struct IEPointerPixel *)to->ie_EventAddress) =
		*((struct IEPointerPixel *)from->ie_EventAddress);
	    break;
	    
	case IESUBCLASS_TABLET :
	    if((to->ie_EventAddress = AllocVec(sizeof(struct IEPointerTablet),
					       MEMF_ANY)) == NULL)
		return FALSE;
	    
	    *((struct IEPointerTablet *)to->ie_EventAddress) =
		*((struct IEPointerTablet *)from->ie_EventAddress);
	    break;
	    
	case IESUBCLASS_NEWTABLET :
	    if((to->ie_EventAddress = AllocVec(sizeof(struct IENewTablet),
					       MEMF_ANY)) == NULL)
		return FALSE;
	    
	    *((struct IENewTablet *)to->ie_EventAddress) =
		*((struct IENewTablet *)from->ie_EventAddress);
	    break;
	    
	default :
	    break;
	}
    }

    return TRUE;
}


AROS_UFH2(struct InputEvent *, cxIHandler,
	  AROS_UFHA(struct InputEvent *     , events, A0),
	  AROS_UFHA(struct CommoditiesBase *, CxBase, A1)
	  )
{
    return CxTree(events, CxBase);
}
