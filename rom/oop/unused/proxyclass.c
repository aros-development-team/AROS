/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for proxy objects.
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>
#include <oop/proxy.h>
#include <oop/server.h>

#include <string.h>

#include "intern.h"

#undef DEBUG
#undef SDEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define OOPBase (GetOBase(((OOP_Class *)cl)->UserData))

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
    OOP_Object *RealObject;

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
static OOP_Object *_Root_New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    /* Get the attributes */
    OOP_Object *realobject;
    struct MsgPort *serverport;
    
    EnterFunc(bug("Proxy::New()\n"));
    
    /* Pares params */

    /* Object from other process which we create a proxy for */
    realobject = (OOP_Object *)GetTagData(aProxy_RealObject,	NULL, msg->attrList);
    
    /* MsgPort to pass method invocation throgh.
       Note that one could very well use a socket or a pipe to pass
       the methods
    */
    
    serverport = (struct MsgPort *)GetTagData(aProxy_Port, 	NULL, msg->attrList);
    
    /* Those two params MUST be supplied */
    if ( !(realobject && serverport) )
    	return (NULL);
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct ProxyData *data;
	ULONG disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    
	data = (struct ProxyData *)OOP_INST_DATA(cl, o);
	
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

	    	ReturnPtr("Proxy::New", OOP_Object *, o);

	    }
	    DeleteMsgPort(data->ReplyPort);
	    
	}
	
	OOP_CoerceMethod(cl, o, (OOP_Msg)&disp_mid);
	
    }
    ReturnPtr("Proxy::New", OOP_Object *, NULL);
}

/****************
**  Dispose()  **
****************/
static VOID _Root_Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ProxyData *data = OOP_INST_DATA(cl, o);
    
    /* Delete the replyport.. */
    if (data->ReplyPort)
    	DeleteMsgPort(data->ReplyPort);
	
    /* .. the message struct.. */
    if (data->Message)
    	FreeMem(data->Message, sizeof (struct ProxyMsg));

    /* .. and the object itself. */
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("Proxy::Dispose");
}

#undef OOPBase
#define OOPBase ((struct Library *)(OOP_OCLASS(o)->UserData))

/*****************
**  DoMethod()  **
*****************/
/* The proxy has a custom DoMethod() call that
   handles passing the method on to the server's MsgPort.
*/   

static IPTR _Proxy_DoMethod(OOP_Object *o, OOP_Msg msg)
{
    struct ProxyData *data = OOP_INST_DATA(OOP_OCLASS(o), o);
    
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

OOP_Class *init_proxyclass(struct Library *OOPBase)
{

    struct OOP_MethodDescr root_methods[] =
    {
	{(IPTR (*)())_Root_New,			moRoot_New},
	{(IPTR (*)())_Root_Dispose,		moRoot_Dispose},
	{ NULL, 0UL }
    };
    
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
    	{ root_methods,		IID_Root, 2},
	{ NULL, 0UL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {aMeta_SuperID,			(IPTR)CLID_Root},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_ID,			(IPTR)CLID_Proxy},
	{aMeta_InstSize,		(IPTR)sizeof (struct ProxyData) },
	{aMeta_DoMethod,		(IPTR)_Proxy_DoMethod},
	{TAG_DONE, 0UL}
    };

    
    OOP_Class *cl;
    
    EnterFunc(bug("InitProxyClass()\n"));
    
    cl = (OOP_Class *)OOP_NewObject(NULL, CLID_MIMeta, tags);
    if (cl)
    {
        cl->UserData = OOPBase;
    	OOP_AddClass(cl);
    }
    
    ReturnPtr ("InitProxyClass", OOP_Class *, cl);
}
