/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: OOP rootclass
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <oop/root.h>
#include <string.h>

#include "intern.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define OOPBase	(GetOBase(root_cl->UserData))

/************************
**  Rootclass methods  **
************************/
struct RootData
{
    ULONG RefCount;
/*    struct InvocationObject *InvObjList;*/ /* List of method objects */
};


#define NUMROOTMETHODS 2

Object *_Root_New(struct IntClass *root_cl, struct IntClass *cl, struct P_Root_New *param)
{
    struct _Object *o;
    struct RootData *data;

    EnterFunc(bug("Root::New(cl=%s, param = %p)\n",
    	cl->PPart.ClassNode.ln_Name, param));
    
    /* Allocate memory for the object */
    D(bug("Object size: %ld\n", cl->PPart.InstOffset + cl->PPart.InstSize + sizeof (struct _Object)));
    o = AllocVec(cl->PPart.InstOffset + cl->PPart.InstSize + sizeof (struct _Object), MEMF_ANY);
    if (o)
    {
    	D(bug("Mem allocated: %p\n", o));
    	o->o_Class = (Class *)cl;
    	
    	data = (struct RootData *)BASEOBJECT(o);
    	
    	cl->ObjectCount ++;
    	
    	ReturnPtr ("Root::New", Object *, BASEOBJECT(o) );
    }
    
    ReturnPtr ("Root::New", Object *, NULL);
}

VOID _Root_Dispose(struct IntClass *root_cl, Object *o, Msg msg)
{
    EnterFunc(bug("Root::Dispose(o=%p, oclass=%s)\n", o, _OBJECT(o)->o_Class->ClassNode.ln_Name));

    ((struct IntClass *)_OBJECT(o)->o_Class)->ObjectCount --;
    D(bug("Object mem: %p, size: %ld\n", _OBJECT(o), ((ULONG *)_OBJECT(o))[-1] ));
    FreeVec(_OBJECT(o));
    
    ReturnVoid("Root::Dispose");
}

#undef OOPBase

BOOL InitRootClass(struct IntOOPBase *OOPBase)
{
    struct MethodDescr mdescr[]=
    {
    	{ (IPTR (*)())_Root_New,		MIDX_Root_New		},
    	{ (IPTR (*)())_Root_Dispose,		MIDX_Root_Dispose	},
	{ NULL, 0UL }
    };

    struct InterfaceDescr _Root_Descr[] =
    {
    	{mdescr, GUID_Root, 2},
	{NULL, NULL, 0UL}
    };
    
    struct IntClass *RootClass = &(OOPBase->ob_RootClass);
    
    RootClass->SuperClass = NULL;
    if (AllocDispatchTables(RootClass, _Root_Descr, OOPBase))
    {
    	RootClass->PPart.ClassNode.ln_Name   	= ROOTCLASS;
	RootClass->PPart.InstOffset 	 	= 0UL;
	RootClass->PPart.InstSize   	 	= 0UL;
	RootClass->SubClassCount 		= 0UL;
	RootClass->ObjectCount	 		= 0UL;

	RootClass->NumInterfaces 		= 1UL;
	RootClass->UserData	 		= (APTR)OOPBase;
	
	AddHead((struct List *)&OOPBase->ob_ClassList,
		(struct Node *)RootClass);
	return (TRUE);
    }
    return (FALSE);
}
