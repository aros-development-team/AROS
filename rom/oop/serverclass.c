/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Class for server objects.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <oop/root.h>
#include <oop/meta.h>
#include <oop/server.h>
#include <oop/proxy.h>
#include <string.h>

#include "intern.h"

#define DEBUG 0
#include <aros/debug.h>

struct ServerData
{
    struct List ObjectList;
    struct MsgPort *ReceivePort;
    struct SignalSemaphore ObjectListLock;

};

struct  ServerObjectNode
{
    struct Node so_Node;
    Object *so_Object;
    
};

#define OOPBase ((struct Library *)cl->UserData)

static Object *_Root_New(Class *cl, Object *o, struct P_Root_New *msg)
{
    EnterFunc(bug("Server::New()\n"));
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct ServerData *data = INST_DATA(cl, o);
	ULONG disp_mid = M_Root_Dispose;
	
	/* Clear so we can test what resources are allocated in Dispose() */
	memset(data, 0, sizeof (struct ServerData));
	NEWLIST(&data->ObjectList);
	InitSemaphore(&data->ObjectListLock);
	
	data->ReceivePort = CreateMsgPort();
	if (data->ReceivePort)
	{
	
	    ReturnPtr("Server::New", Object *, o);
	}
	CoerceMethodA(cl, o, (Msg)&disp_mid);
	
    }
    ReturnPtr ("Server::New", Object *, NULL);
}

static VOID _Root_Dispose(Class *cl, Object *o, Msg msg)
{
    struct Node *node, *nextnode;
    struct ServerData *data = INST_DATA(cl, o);
    
    EnterFunc(bug("Server::Dispose()\n"));
    
    node = data->ObjectList.lh_Head;
    
    while ((nextnode = node->ln_Succ))
    {
    	FreeVec(node->ln_Name);
	FreeMem(node, sizeof (struct ServerObjectNode));
	
	node = nextnode;
    }
    
    if(data->ReceivePort)
    	DeleteMsgPort(data->ReceivePort);
    
    ReturnVoid("Server::Dispose");
}


/*****************************
**  IServer implementation  **
*****************************/

static BOOL _Server_AddObject(Class *cl, Object *o, struct P_Server_AddObject *msg)
{
    struct ServerData *data = INST_DATA(cl, o);
     
    struct ServerObjectNode *so;
    
    EnterFunc(bug("Server::AddObject(obj=%p, id=%s)\n", msg->Object, msg->ObjectID));
     
    so = AllocMem(sizeof (struct ServerObjectNode), MEMF_ANY);
    if (so)
    {
        so->so_Node.ln_Name = AllocVec(strlen(msg->ObjectID) + 1, MEMF_ANY);
	if (so->so_Node.ln_Name)
	{
	    strcpy(so->so_Node.ln_Name, msg->ObjectID);
	    so->so_Object = msg->Object;
	    
	    ObtainSemaphore(&data->ObjectListLock);
	    AddTail(&data->ObjectList, (struct Node *)so);
	    ReleaseSemaphore(&data->ObjectListLock);
	    
	    ReturnBool ("Server::AddObject", TRUE);
	}
	FreeMem(so, sizeof (struct ServerObjectNode));
    }
    ReturnBool ("Server::AddObject", FALSE);
}

static VOID _Server_RemoveObject(Class *cl, Object *o, struct P_Server_RemoveObject *msg)
{
    struct ServerData *data = INST_DATA(cl, o);
   
    struct ServerObjectNode *so;
   
    EnterFunc(bug("Server::RemoveObject(objid=%s)\n", msg->ObjectID));

    ObtainSemaphore(&data->ObjectListLock);
    so = (struct ServerObjectNode *)FindName(&data->ObjectList, msg->ObjectID);
    if (so)
    {
   	Remove((struct Node *)so);
   	FreeVec(so->so_Node.ln_Name);
	FreeMem(so, sizeof (struct ServerObjectNode) );
    }
    ReleaseSemaphore(&data->ObjectListLock);

    ReturnVoid("Server::RemoveObject");
}

static Object * _Server_FindObject(Class *cl, Object *o, struct P_Server_FindObject *msg)
{
    struct ServerObjectNode *so;
    struct ServerData *data = INST_DATA(cl, o);
    
    EnterFunc(bug("Server::FindObject(objid=%s)\n", msg->ObjectID));
    
    /* See if the server knows about the object */
    ObtainSemaphoreShared(&data->ObjectListLock);
    so = (struct ServerObjectNode *)FindName(&data->ObjectList, msg->ObjectID);
   ReleaseSemaphore(&data->ObjectListLock);

    if (so)
    {
    	/* Create a proxy for the object */
	struct TagItem proxy_tags[] =
	{
	    {A_Proxy_RealObject,	(IPTR)so->so_Object},
	    {A_Proxy_Port,		(IPTR)data->ReceivePort},
	    {TAG_DONE,	0UL}
	};
	
	Object *proxy;
	
	proxy = NewObjectA(NULL, PROXYCLASS, proxy_tags);
	if (proxy)
	{
	    ReturnPtr("Server::FindObject", Object *, proxy);
	}
    }
    
    ReturnPtr("Server::FindObject", Object *, NULL);
}

static VOID _Server_Run(Class *cl, Object *o, Msg msg)
{
     struct ServerData *data = INST_DATA(cl, o);
     
     /* Wait for something to happen */
     for (;;)
     {
     	struct ProxyMsg *pm;
     	WaitPort(data->ReceivePort);
	
	while ( (pm = (struct ProxyMsg *)GetMsg(data->ReceivePort)) )
	{
	    /* Execute method */
	    pm->pm_RetVal = DoMethod(pm->pm_Object, pm->pm_ObjMsg);
	    
	    ReplyMsg((struct Message *)pm);
	}
	
     }
     
     return;
}

#undef OOPBase

Class *InitServerClass(struct Library *OOPBase)
{

    struct MethodDescr root_methods[] =
    {
	{(IPTR (*)())_Root_New,			MIDX_Root_New},
	{(IPTR (*)())_Root_Dispose,		MIDX_Root_Dispose},
	{ NULL, 0UL }
    };
    
    struct MethodDescr server_methods[] =
    {
	{(IPTR (*)())_Server_AddObject,		MIDX_Server_AddObject},
	{(IPTR (*)())_Server_RemoveObject,	MIDX_Server_RemoveObject},
	{(IPTR (*)())_Server_FindObject,	MIDX_Server_FindObject},
	{(IPTR (*)())_Server_Run,		MIDX_Server_Run},
	{ NULL, 0UL }
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{ root_methods,		GUID_Root, 2},
    	{ server_methods,	GUID_Server, 4},
	{ NULL, 0UL, 0UL}
    };
    
    struct TagItem tags[] =
    {
        {A_Class_SuperID,		(IPTR)ROOTCLASS},
	{A_Class_InterfaceDescr,	(IPTR)ifdescr},
	{A_Class_ID,			(IPTR)SERVERCLASS},
	{A_Class_InstSize,		(IPTR)sizeof (struct ServerData)},
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
