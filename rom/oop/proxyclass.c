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


struct ProxyData
{
    Object *RealObject;
    struct MsgPort *ServerPort;
    struct MsgPort *ReplyPort;
    struct ProxyMsg *Message;
};

/*************************
**  ProxyClass methods  **
*************************/

static Object *_Root_New(Class *cl, Object *o, struct P_Root_New *msg)
{
    /* Get the attributes */
    Object *realobject;
    struct MsgPort *serverport;
    
    EnterFunc(bug("Proxy::New()\n"));
    
    realobject = (Object *)GetTagData(A_Proxy_RealObject,	NULL, msg->AttrList);
    serverport = (struct MsgPort *)GetTagData(A_Proxy_Port, 	NULL, msg->AttrList);
    
    
    if ( !(realobject && serverport) )
    	return (NULL);
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
        struct ProxyData *data;
	ULONG disp_mid = M_Root_Dispose;
	    
	data = (struct ProxyData *)INST_DATA(cl, o);
	
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
		
	    	data->RealObject = realobject;
	    	data->ServerPort = serverport;
		
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

static VOID _Root_Dispose(Class *cl, Object *o, Msg msg)
{
    struct ProxyData *data = INST_DATA(cl, o);
    
    if (data->ReplyPort)
    	DeleteMsgPort(data->ReplyPort);
	
    if (data->Message)
    	FreeMem(data->Message, sizeof (struct ProxyMsg));
    
    ReturnVoid("Proxy::Dispose");
}


/*****************
**  DoMethod()  **
*****************/
#undef OOPBase
#define OOPBase ((struct Library *)(OCLASS(o)->UserData))
static IPTR _Proxy_DoMethod(Object *o, Msg msg)
{
    struct ProxyData *data = INST_DATA(cl, o);
    
    EnterFunc(bug("Proxy_DoMethod()\n"));
    
    data->Message->pm_ObjMsg = msg;
    
    /* Send to server */
    PutMsg(data->ServerPort, (struct Message *)data->Message);
    
    /* Wait for server to reply */
    WaitPort(data->ReplyPort);
    
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
