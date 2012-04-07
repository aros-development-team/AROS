/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Commodities input handler
    Lang: English
*/

/*  INTERNALS

  * Currently, there is no protection against stupid users. If you, for
    instance, install a broker and attach a custom object to it, and this
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
#include <string.h>

#define DEBUG_CXTREE(x)		;//if ((msg->cxm_Data->ie_Class == IECLASS_RAWMOUSE) && (msg->cxm_Data->ie_Code != 255)) { x; }
#define DEBUG_TRANSFUNC(x)	;
#define DEBUG_SENDFUNC(x)	;
#define DEBUG_COPYIEVENT(x)	x;

#define  SUPERDEBUG 0
#define  DEBUG 0
#include <aros/debug.h>

static void ProduceEvent(CxMsg *, struct CommoditiesBase *CxBase);
static void DebugFunc(CxMsg *, CxObj *, struct CommoditiesBase *CxBase);
static void TransFunc(CxMsg *, CxObj *, struct CommoditiesBase *CxBase);
static void SendFunc(CxMsg *, CxObj *, struct CommoditiesBase *CxBase);
BOOL CopyInputEvent(struct InputEvent *from, struct InputEvent *to, struct CommoditiesBase *CxBase);

#ifdef __mc68000
extern void cx_Thunk(void);
asm (
    ".global cx_Thunk\n"
    "cx_Thunk:\n"
        "movem.l %a0-%a1,%sp@-\n"
        "jsr.l  %a2@\n"
        "addq.l #8,%sp\n"
        "rts\n"
    );
#endif

AROS_UFH2(struct InputEvent *, CxTree,
    AROS_UFHA(struct InputEvent *     , events , A0),
    AROS_UFHA(struct CommoditiesBase *, CxBase , A6))
{
    AROS_USERFUNC_INIT

    CxObj *co;
    CxMsg *tempMsg, *msg;
    struct Node *node, *succ;

    if (events == NULL)
    {
	return NULL;
    }

    ObtainSemaphore(&CxBase->cx_SignalSemaphore);

    if (IsListEmpty(&CxBase->cx_BrokerList))
    {
	ReleaseSemaphore(&CxBase->cx_SignalSemaphore);

	return events;
    }

    /* Take care of the processed input events */
    ForeachNodeSafe(&CxBase->cx_GeneratedInputEvents, node, succ)
    {
        struct GeneratedInputEvent *tempEvent;

        tempEvent = (struct GeneratedInputEvent *)(((UBYTE *)node) - offsetof(struct GeneratedInputEvent, node));
        FreeCxStructure(tempEvent, CX_INPUTEVENT, (struct Library *)CxBase);
    }

    CxBase->cx_IEvents = NULL;
    CxBase->cx_EventExtra = &CxBase->cx_IEvents;
    NEWLIST(&CxBase->cx_GeneratedInputEvents);

    /* Free all the replied messages */
    while ((tempMsg = (CxMsg *)GetMsg(&CxBase->cx_MsgPort)) != NULL)
    {
	FreeCxStructure(tempMsg, CX_MESSAGE, (struct Library *)CxBase);
    }

#if SUPERDEBUG
    {
	CxObj *node;

	kprintf("List of brokers:\n");

	ForeachNode(&CxBase->cx_BrokerList, node)
	{
	    if (CXOBJType(node) == CX_BROKER)
	    {
		kprintf("B: %s\n", node->co_Ext.co_BExt->bext_Name);
	    }
	    else
	    {
		kprintf("Something else\n");
	    }
	}
    }
#endif

    /* Route all messages to the first broker */

    co = (CxObj *)GetHead(&CxBase->cx_BrokerList);

#if SUPERDEBUG
    kprintf("Initial broker: %s\n", co->co_Ext.co_BExt->bext_Name);
#endif

    ForeachNode(&CxBase->cx_MessageList, tempMsg)
    {
	ROUTECxMsg(tempMsg, co);
    }

    AddIEvents(events); /* Add the new events (incapsulated in commodtity
			   messages) to the message list. */

/***************************************************************************/

    /* Process the new events */

    while ((msg = (CxMsg *)GetHead(&CxBase->cx_MessageList)) != NULL)
    {
	co = msg->cxm_Routing;

	DEBUG_CXTREE(dprintf("CxTree: Msg %p Object %p\n", msg, co));

	while (co == NULL && msg->cxm_Level != 0)
	{
	    // kprintf("Next level %i\n", msg->cxm_Level - 1);

	    msg->cxm_Level--;
	    co = msg->cxm_retObj[msg->cxm_Level];
	    co = (CxObj *)GetSucc(&co->co_Node);

	    // kprintf("Found return object %p\n", co);

	    // if (CXOBJType(co) == CX_BROKER)
	    // {
	    //     kprintf("Returnobj (broker) = %s\n",
	    //	       co->co_Ext.co_BExt->bext_Name);
	    // }
	}

	/* If there are no more objects that shall process the event, we
	   link it in to the list of ready input events */

	if (co == NULL)
	{
	    ProduceEvent(msg, CxBase);
	    continue;
	}

#if DEBUG
	DEBUG_CXTREE(
	{
	    if(CXOBJType(co) == CX_BROKER)
		kprintf("Broker: %s\n", co->co_Ext.co_BExt->bext_Name);

	    if(co->co_Node.ln_Succ != NULL &&
	       co->co_Node.ln_Succ->ln_Type == CX_BROKER)
		kprintf("Routing to next broker %s (this broker=%s) %p\n",
			((CxObj *)(co->co_Node.ln_Succ))->co_Ext.co_BExt->bext_Name,
			co->co_Ext.co_BExt->bext_Name,
			co);
	})
#endif

	/* Route the message to the next object */

	ROUTECxMsg(msg, (CxObj *)GetSucc(&co->co_Node));

	if (!(co->co_Flags & COF_ACTIVE))
	{
	    continue;
	}

	DEBUG_CXTREE(dprintf("CxTree: Object %p Type %d\n", co, CXOBJType(co)));

	switch (CXOBJType(co))
	{
	case CX_INVALID:
	    DEBUG_CXTREE(dprintf("CxTree: CX_INVALID\n"));
	    break;

	case CX_FILTER:
	    DEBUG_CXTREE(dprintf("CxTree: CX_FILTER\n"));

	    DEBUG_CXTREE(dprintf("CxTree: Filter 0x%lx\n",
				msg->cxm_Type));

	    if ((co->co_Error & COERR_BADFILTER))
	    {
		DEBUG_CXTREE(dprintf("CxTree: bad filter!\n"));
		break;
	    }

	    if (msg->cxm_Type == CXM_IEVENT)
	    {
		DEBUG_CXTREE(dprintf("CxTree: Data 0x%lx FilterIX 0x%lx\n",
				msg->cxm_Data,
				co->co_Ext.co_FilterIX));
		if (MatchIX(msg->cxm_Data, co->co_Ext.co_FilterIX) != 0)
		{
		    DEBUG_CXTREE(dprintf("CxTree: filter matched\n"));
		    DivertCxMsg(msg, co, co);
		}
		else
		{
		    DEBUG_CXTREE(dprintf("CxTree: filter not matched\n"));
		}
	    }
	    else
	    {
		DEBUG_CXTREE(dprintf("CxTree: no CXM_EVENT\n"));
	    }
	    break;

	case CX_TYPEFILTER:
	    DEBUG_CXTREE(dprintf("CxTree: CX_TYPEFILTER\n"));
	    if ((msg->cxm_Type & co->co_Ext.co_TypeFilter) != 0)
	    {
		DEBUG_CXTREE(dprintf("CxTree: hit\n"));
		DivertCxMsg(msg, co, co);
	    }
	    break;

	case CX_SEND:
	    DEBUG_CXTREE(dprintf("CxTree: CX_SEND\n"));
	    SendFunc(msg, co, CxBase);
	    break;

	case CX_SIGNAL:
	    DEBUG_CXTREE(dprintf("CxTree: CX_SIGNAL Task 0x%lx <%s>\n",
				co->co_Ext.co_SignalExt->sixt_Task,
				co->co_Ext.co_SignalExt->sixt_Task->tc_Node.ln_Name));
	    Signal(co->co_Ext.co_SignalExt->sixt_Task,
		   1 << co->co_Ext.co_SignalExt->sixt_SigBit);
	    break;

	case CX_TRANSLATE:
	    DEBUG_CXTREE(dprintf("CxTree: CX_TRANSLATE\n"));
	    TransFunc(msg, co, CxBase);
	    break;

	case CX_BROKER:
	    DEBUG_CXTREE(dprintf("CxTree: CX_BROKER\n"));
	    DivertCxMsg(msg, co, co);
	    break;

	case CX_DEBUG:
	    DEBUG_CXTREE(dprintf("CxTree: CX_DEBUG\n"));
	    DebugFunc(msg, co, CxBase);
	    break;

	case CX_CUSTOM:
	    DEBUG_CXTREE(dprintf("CxTree: CX_CUSTOM\n"));
	    msg->cxm_ID = co->co_Ext.co_CustomExt->cext_ID;

	    DEBUG_CXTREE(dprintf("CxTree: Action 0x%lx\n",
				co->co_Ext.co_CustomExt->cext_Action));
	    /* Action shouldn't be NULL, but well, sometimes it is...
	     */
	    if (co->co_Ext.co_CustomExt->cext_Action)
	    {
#ifdef __mc68000
		/* The autodocs suggest the arguments should be passed on the stack.
		 * But they were also in a0/a1 and some things seem to rely on that.
		 * Let's also pass CxBase in a6 just in case.
		 *
		 * All other architectures (should) be using the stack.
		 */
		AROS_UFC4(void, cx_Thunk,
		      AROS_UFCA(CxMsg*, msg, A0),
		      AROS_UFCA(CxObj*, co, A1),
		      AROS_UFCA(APTR, co->co_Ext.co_CustomExt->cext_Action, A2),
		      AROS_UFCA(struct CommoditiesBase *, CxBase, A6));
#else
#ifdef __MORPHOS__
		REG_A7 -= 8;
		*(CxMsg**)REG_A7 = msg;
		*(CxObj**)(REG_A7 + 4) = co;
#endif
		AROS_UFC3(void, co->co_Ext.co_CustomExt->cext_Action,
		      AROS_UFCA(CxMsg*, msg, A0),
		      AROS_UFCA(CxObj*, co, A1),
		      AROS_UFCA(struct CommoditiesBase *, CxBase, A6));
#ifdef __MORPHOS__
		REG_A7 += 8;
#endif
#endif
	    }
	    break;

	case CX_ZERO:
	    DEBUG_CXTREE(dprintf("CxTree: CX_ZERO\n"));
	    ProduceEvent(msg, CxBase);
	    break;
	}

	DEBUG_CXTREE(dprintf("CxTree: done\n"));
    }

    ReleaseSemaphore(&CxBase->cx_SignalSemaphore);

    return CxBase->cx_IEvents;

    AROS_USERFUNC_EXIT
}


static void ProduceEvent(CxMsg *msg, struct CommoditiesBase *CxBase)
{
    struct GeneratedInputEvent *temp;

    if ((temp = (struct GeneratedInputEvent *)AllocCxStructure(CX_INPUTEVENT, 0,
	       (struct Library *)CxBase)) != NULL)
    {
        if (!CopyInputEvent(msg->cxm_Data, &temp->ie, CxBase))
	{
	    DEBUG_COPYIEVENT(dprintf("ProduceEvent: CopyInputEvent() failed!\n"));
	}

	/* Put the input event last in the ready list and update bookkeeping */
	temp->ie.ie_NextEvent = NULL;

	*(CxBase->cx_EventExtra) = &temp->ie;
	CxBase->cx_EventExtra = &temp->ie.ie_NextEvent;

	AddTail((struct List *)&CxBase->cx_GeneratedInputEvents,
		(struct Node *)&temp->node);
    }

    DisposeCxMsg(msg);
}


static void SendFunc(CxMsg *msg, CxObj *co, struct CommoditiesBase *CxBase)
{
    CxMsg  *tempMsg;
    struct InputEvent *saveIE;  /* To save the InputEvent pointer
				   from being destroyed by CopyMem() */

    DEBUG_SENDFUNC(dprintf("SendFunc: msg %p co %p MsgPort %p ID 0x%lx\n",
			    msg, co, co->co_Ext.co_SendExt->sext_MsgPort, co->co_Ext.co_SendExt->sext_ID));

    if (co->co_Ext.co_SendExt->sext_MsgPort == NULL)
    {
	return;
    }

    tempMsg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE,
					(struct Library *)CxBase);

    if (tempMsg == NULL)
    {
	DEBUG_SENDFUNC(dprintf("SendFunc: failed!\n"));
	return;
    }

    saveIE = tempMsg->cxm_Data;
    CopyMem(msg, tempMsg, sizeof(CxMsg));
    tempMsg->cxm_Data = saveIE;

    if (!CopyInputEvent(msg->cxm_Data, tempMsg->cxm_Data, CxBase))
    {
    DEBUG_COPYIEVENT(dprintf("SendFunc: CopyInputEvent() failed!\n"));
    }

    tempMsg->cxm_ID = co->co_Ext.co_SendExt->sext_ID;

    PutMsg(co->co_Ext.co_SendExt->sext_MsgPort, (struct Message *)tempMsg);
}


static void TransFunc(CxMsg *msg, CxObj *co, struct CommoditiesBase *CxBase)
{
    struct  InputEvent *event;
    CxMsg              *msg2;

    DEBUG_TRANSFUNC(dprintf("TransFunc: msg %p co %p ie %p\n",
			    msg, co, co->co_Ext.co_IE));

    if (co->co_Ext.co_IE != NULL)
    {
        event = co->co_Ext.co_IE;

	do
	{
	    struct InputEvent *saveIE;  /* To save the InputEvent pointer
					   from being destroyed by CopyMem() */

	    DEBUG_TRANSFUNC(dprintf("TransFunc: Generate class %d code 0x%x\n",
				    event->ie_Class, event->ie_Code));

	    if ((msg2 = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE,
				(struct Library *)CxBase)) == NULL)
	    {
		DEBUG_TRANSFUNC(dprintf("TransFunc: failed!\n"));
		break;
	    }

	    saveIE = msg2->cxm_Data;
	    CopyMem(msg, msg2, sizeof(CxMsg));
	    msg2->cxm_Data = saveIE;

	    /* Don't care about errors for now */
	    if (!CopyInputEvent(event, msg2->cxm_Data, CxBase))
	    {
		DEBUG_COPYIEVENT(dprintf("TransFunc: CopyInputEvent() failed!\n"));
	    }

	    AddHead(&CxBase->cx_MessageList, (struct Node *)msg2);

	} while ((event = event->ie_NextEvent) != NULL);
    }

    DisposeCxMsg(msg);
}


static void DebugFunc(CxMsg *msg, CxObj *co, struct CommoditiesBase *CxBase)
{
    kprintf("\n----\nDEBUG NODE: %lx, ID: %lx\n"
	    "\tCxMsg: %lx, type: %x, data %lx destination %lx\n",
	    co, co->co_Ext.co_DebugID, msg->cxm_Routing, msg->cxm_Data,
	    msg->cxm_Type);

    if (msg->cxm_Type != CXM_IEVENT)
    {
	return;
    }

    kprintf("dump IE: %lx\n"
	    "\tClass %lx"
	    "\tCode %lx"
	    "\tQualifier %lx"
	    "\nEventAddress %lx",
	    msg->cxm_Data, msg->cxm_Data->ie_Class, msg->cxm_Data->ie_Code,
	    msg->cxm_Data->ie_Qualifier, msg->cxm_Data->ie_EventAddress);
}


BOOL CopyInputEvent(struct InputEvent *from, struct InputEvent *to,
			   struct CommoditiesBase *CxBase)
{
    bcopy(from, to, sizeof(struct InputEvent));

    if (from->ie_Class == IECLASS_NEWPOINTERPOS)
    {
	switch (from->ie_SubClass)
	{
	case IESUBCLASS_PIXEL :
	    if ((to->ie_EventAddress = AllocVec(sizeof(struct IEPointerPixel),
						MEMF_ANY)) == NULL)
	    {
		return FALSE;
	    }

	    bcopy(from->ie_EventAddress, to->ie_EventAddress, sizeof(struct IEPointerPixel));
	    break;

	case IESUBCLASS_TABLET :
	    if ((to->ie_EventAddress = AllocVec(sizeof(struct IEPointerTablet),
						MEMF_ANY)) == NULL)
	    {
		return FALSE;
	    }

	    bcopy(from->ie_EventAddress, to->ie_EventAddress, sizeof(struct IEPointerTablet));
	    break;

	case IESUBCLASS_NEWTABLET :
	    if ((to->ie_EventAddress = AllocVec(sizeof(struct IENewTablet),
						MEMF_ANY)) == NULL)
	    {
		return FALSE;
	    }

	    bcopy(from->ie_EventAddress, to->ie_EventAddress, sizeof(struct IENewTablet));
	    break;

	default :
	    break;
	}
    }

    return TRUE;
}


AROS_UFH2(struct InputEvent *, cxIHandler,
	  AROS_UFHA(struct InputEvent *     , events, A0),
	  AROS_UFHA(struct CommoditiesBase *, CxBase, A1))
{
    AROS_USERFUNC_INIT

    return AROS_UFC2(struct InputEvent *, AROS_ASMSYMNAME(CxTree),
	      AROS_UFCA(struct InputEvent *     , events , A0),
	      AROS_UFCA(struct CommoditiesBase *, CxBase , A6));

    AROS_USERFUNC_EXIT
}
