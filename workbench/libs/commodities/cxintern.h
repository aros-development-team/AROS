/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/


#ifndef COMMODITIES_BASE_H
#define COMMODITIES_BASE_H

#define COF_VALID  (1<<0)	/* Object is inserted in a commodity list */

#define cxm_MaxLevel 32		/* Maximum routing level */

enum { CX_OBJECT, CX_MESSAGE, CX_INPUTEVENT };
enum { CXM_SINGLE, CXM_DOUBLE };

#define  AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>

#include <exec/types.h>
#include <exec/io.h>
#include <exec/nodes.h>

#include <exec/ports.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <libraries/commodities.h>
#include <dos/dos.h>
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif

/* Note that this is structurally the same as the BrokerCopy
 * in commodities.h and make sure to keep it so as
 * GetBrokerList() currently depends on this. */
struct BrokerExt
{
    char            bext_Name[CBD_NAMELEN];
    char            bext_Title[CBD_TITLELEN];
    char            bext_Descr[CBD_DESCRLEN];
    struct Task    *bext_Task;
    struct MsgPort *bext_MsgPort;
};

struct SendExt
{
    struct MsgPort *sext_MsgPort;
    ULONG           sext_ID;
};

struct SignalExt
{
    struct Task *sixt_Task;
    UBYTE        sixt_SigBit;
};

struct CustomExt
{
    VOID  (*cext_Action)(VOID);
    ULONG   cext_ID;
};


typedef struct cx_Object
{
    struct   Node     co_Node;
    UBYTE             co_Flags;
    UBYTE             co_Error;
    struct   MinList  co_ObjList;
    
    union
    {
	ULONG                co_DebugID;
	ULONG                co_TypeFilter;
	struct InputEvent   *co_IE;		/* Translate */
	IX		    *co_FilterIX;
	struct BrokerExt    *co_BExt;
	struct SendExt      *co_SendExt;
	struct SignalExt    *co_SignalExt;
	struct CustomExt    *co_CustomExt;
    } co_Ext;
} CxObj;

typedef struct cx_Message
{
    struct Message     cxm_Message;
    CxObj             *cxm_Routing;           /* Next destination */
    LONG	       cxm_ID;
    UBYTE	       cxm_Type;
    UBYTE	       cxm_Level;
    CxObj             *cxm_retObj[cxm_MaxLevel];
    struct InputEvent *cxm_Data;
} CxMsg;


struct GeneratedInputEvent
{
    struct InputEvent ie;
    struct MinNode    node;
};

struct CommoditiesBase
{
    struct Library          cx_Lib;
    
    struct Library         *cx_TimerBase;
    
    struct IOStdReq         cx_IORequest;     /* To set up input handler */
    struct Interrupt        cx_Interrupt;     /* Input handler */
    struct MsgPort          cx_InputMP;       /* Reply port for input.device */
    struct List             cx_BrokerList;
    struct List             cx_MessageList;
    struct List		    cx_GeneratedInputEvents;
    struct SignalSemaphore  cx_SignalSemaphore;
    struct InputEvent      *cx_IEvents;
    struct InputEvent     **cx_EventExtra;    /* Only for bookkeeping
						 purposes */
    struct MsgPort          cx_MsgPort;
    BOOL                    cx_Running;       /* Is the input handler
						   installed? */
    struct timerequest      cx_TimerIO;	      /* For timer.device... */
    struct MsgPort          cx_TimerMP;	      /* ... communication */
};

/* Extra prototypes */

BOOL  SetupIHandler(struct CommoditiesBase *CxBase);
VOID  FreeCxStructure(APTR obj, int type, struct Library *CxBase);
APTR  AllocCxStructure(LONG type, LONG objtype, struct Library *CxBase);
ULONG CheckStatus(CxObj *broker, ULONG command, struct Library *CxBase);


/* Locate library bases */

#ifdef TimerBase
#undef TimerBase
#endif

#define TimerBase ((struct CommoditiesBase *)CxBase)->cx_TimerBase

#define ROUTECxMsg(msg, obj)    msg->cxm_Routing = obj
#define CXOBJType(co)           (co->co_Node.ln_Type)

#define GPB(x) ((struct CommoditiesBase *)x)

#ifndef __MORPHOS__
#define dprintf bug
#endif

#endif /* COMMODITIES_BASE_H */
