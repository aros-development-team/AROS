/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Class for proxy objects.
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>
#include <oop/root.h>
#include <oop/proxy.h>
#include <oop/meta.h>
#include <oop/server.h>

#include <string.h>

#include "intern.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define OOPBase (GetOBase(((Class *)cl)->UserData))

/* This proxyclass is used to create a proxy object for
   another object owned by another process. You can then call
   methods on the object through the proxy, using IPC.
   This makes methodcalls threadsafe.

   Note the API of this class sucks (It's not OS-independent).
   This implementation is  mainly there to show that the idea works.

*/

struct ProxyData
{
    /* The object this is a proxy for */
    Object *RealObject;

    /* Server's Msgport to which we send requests */
    struct MsgPort *ServerPort;
    
    /* The server replies to this port */
    struct MsgPort *ReplyPort;
    
    /* Mem used for passing the message */
    struct ProxyMsg *Message;
};


/************
**  New()  **
************/
static Object *_Root_New(Class *cl, Object *o, struct P_Root_New *msg)
{
    /* Get the attributes */
    Object *realobject;
    struct MsgPort *serverport;
    
    EnterFunc(bug("Proxy::New()\n"));
    
    /* Pares params */

    /* Object from other process which we create a proxy for */
    realobject = (Object *)GetTagData(A_Proxy_RealObject,	NULL, msg->AttrList);
    
    /* MsgPort to pass method invocation throgh.
       Note that one could very well use a socket or a pipe to pass
       the methods
    */
    
    serverport = (struct MsgPort *)GetTagData(A_Proxy_Port, 	NULL, msg->AttrList);
    
    /* Those two params MUST be supplied */
    if ( !(realobject && serverport) )
    	return (NULL);
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
        struct ProxyData *data;
	ULONG disp_mid = M_Root_Dispose;
	    
	data = (struct ProxyData *)INST_DATA(cl, o);
	
	/* Clear the instance data */
	memset(data, 0, sizeof (struct ProxyData));
	
	/* This is ugly, as we will soon run out of sigbits */
	data->ReplyPort = CreateMsgPort();
	if (data->ReplyPort)
	{
	    /* Allocate MEMF_PUBLIC message struct */
	    data->Message = AllocMem(sizeof (struct ProxyMsg), MEMF_PUBLIC);
	    if (data->Message)
	    {
	    	struct ProxyMsg *pm = data->Message;
		
		/* Save the instance data */
	    	data->RealObject = realobject;
	    	data->ServerPort = serverport;
		
		/* Preinitialize some fields in the message struct */
    		pm->pm_Message.mn_Length    = sizeof (struct ProxyMsg);
    		pm->pm_Message.mn_ReplyPort = data->ReplyPort;
    		pm->pm_Object = realobject;

	    	ReturnPtr("Proxy::New", Object *, o);

	    }
	    DeleteMsgPort(data->ReplyPort);
	    
	}
	
	CoerceMethodA(cl, o, (Msg)&disp_mid);
	
    }
    ReturnPtr("Proxy::New", Object *, NULL);
}

/****************
**  Dispose()  **
****************/
static VOID _Root_Dispose(Class *cl, Object *o, Msg msg)
{
    struct ProxyData *data = INST_DATA(cl, o);
    
    /* Delete the replyport.. */
    if (data->ReplyPort)
    	DeleteMsgPort(data->ReplyPort);
	
    /* .. the message struct.. */
    if (data->Message)
    	FreeMem(data->Message, sizeof (struct ProxyMsg));

    /* .. and the object itself. */
    DoSuperMethodA(cl, o, msg);
    
    ReturnVoid("Proxy::Dispose");
}

#undef OOPBase
#define OOPBase ((struct Library *)(OCLASS(o)->UserData))

/*****************
**  DoMethod()  **
*****************/
/* The proxy has a custom DoMethod() call that
   handles passing the method on to the server's MsgPort.
*/   

static IPTR _Proxy_DoMethod(Object *o, Msg msg)
{
    struct ProxyData *data = INST_DATA(cl, o);
    
    EnterFunc(bug("Proxy_DoMethod()\n"));
    
    /* Pass a pointer to the message */
    data->Message->pm_ObjMsg = msg;
    
    /* Send to server */
    PutMsg(data->ServerPort, (struct Message *)data->Message);
    
    /* Wait for server to reply. Note that this is prone to deadlocks,
       and that this must be fixed. (For example the task calling
       the remote obj through the proxy, is also server.
       And the server receiving the call tries to call *us*.
    */
    WaitPort(data->ReplyPort);
    
    /* Get the reply, so we can reuse the memory to send new requests.
     */
    GetMsg(data->ReplyPort);
    
    ReturnInt("Proxy_DoMethod", IPTR, data->Message->pm_RetVal);
}


#undef OOPBase

Class *InitProxyClass(struct Library *OOPBase)
{

    struct MethodDescr root_methods[] =
    {
	{(IPTR (*)())_Root_New,			MIDX_Root_New},
	{(IPTR (*)())_Root_Dispose,		MIDX_Root_Dispose},
	{ NULL, 0UL }
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{ root_methods,		GUID_Root, 2},
	{ NULL, 0UL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {A_Class_SuperID,		(IPTR)ROOTCLASS},
	{A_Class_InterfaceDescr,	(IPTR)ifdescr},
	{A_Class_ID,			(IPTR)PROXYCLASS},
	{A_Class_InstSize,		(IPTR)sizeof (struct ProxyData) },
	{A_Class_DoMethod,		(IPTR)_Proxy_DoMethod},
	{TAG_DONE, 0UL}
    };

    
    Class *cl;
    
    EnterFunc(bug("InitServerClass()\n"));
    
    cl = (Class *)NewObjectA(NULL, METACLASS, tags);
    if (cl)
    {
        cl->UserData = OOPBase;
    	AddClass(cl);
    }
    
    ReturnPtr ("InitServerClass", Class *, cl);
}
