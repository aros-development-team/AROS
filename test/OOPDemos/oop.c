/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/


#include "protos.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "oop.h"

#define SDEBUG 0
#define DEBUG 0
#include "debug.h"

/**************************************************/
VOID CleanupOOP();
BOOL InitOOP();
Class *MakeClass(STRPTR classID, STRPTR superID, struct InterfaceDescr *ifDescr,
	ULONG instDataSize);
VOID FreeClass(Class * cl);

/* could go into a library base */
// struct SignalSemaphore ClassListSema;
Class RootClassMem;
struct OOPBase oopbasemem;
struct OOPBase *oopbase = &oopbasemem;

/******************************************************************/

/* The oop system code */

BOOL AllocDispatchTables(Class *cl, struct InterfaceDescr *iDescr);
VOID FreeDispatchTables(Class *cl);

/******************
**  MakeClass()  **
******************/
Class *MakeClass(STRPTR classID, STRPTR superID, struct InterfaceDescr *cl_Descr,
	ULONG instDataSize)
{
    #define UB(x) ((UBYTE *)x)
    
    #define UnlockCL   	 ReleaseSemaphore( &ClassListSema )
    #define LockCLShared ObtainSemaphoreShared( &ClassListSema )
    #define LockCL 	 ObtainSemaphore( &ClassListSema )
    
    #define ClassID ClassNode.ln_Name

    Class *cl, *supercl;

    /* Find superclass */    
    EnterFunc(bug("MakeClass(classID=%s, superID=%s, ifDescr=%p, instDataSize=%ld)\n",
    		classID, superID, cl_Descr, instDataSize));

LockCL;
    supercl = (Class *)FindName( &(oopbase->ClassList), superID);
    if (supercl)
    {
    	/* Mark the class as busy, so it isn't freed while we are allocating
    	** stuff for our class
    	*/
    	supercl->SubClassCount ++;
    }
UnlockCL;

    if (!supercl)
    	return (FALSE);
    	
    D(bug("Found superclass %s\n", supercl->ClassID));
    	
    /* Allocate class structure */
    D(bug("Allocating class of size %d\n", sizeof (Class) ));
    cl = malloc(sizeof (Class));
    if (cl)
    {
    	
    	D(bug("Allocated class structure\n"));
    	
    	cl->ClassID = malloc( strlen(classID) + 1);
    	if (cl->ClassID)
	{
	    D(bug("Allocated class id\n"));
	    strcpy(cl->ClassID, classID);

	    
	    /* Must do this before calling AllocDispatchTables(), so that
	    ** it knows from where to copy dispatch tables.
	    */
	    cl->SuperClass = supercl;
	    if (AllocDispatchTables(cl, cl_Descr))
	    {
    	    	
    	    	/* Update instance data info */
    	    	cl->InstOffset = supercl->InstOffset + supercl->InstSize;
    	    	cl->InstSize = instDataSize;
    	    	
		ReturnPtr("MakeClass", Class *, cl);
    	    	
    	    }

    	    free(cl->ClassID);
    	    
    	}
    	
    	free(cl);
    	
    }
    
    /* Initalization failed, free lock on superclass */
    supercl->SubClassCount --;
    
    ReturnPtr ("MakeClass", Class *, NULL);

} /* CreateClass */



/******************
**  FreeClass()  **
******************/

VOID FreeClass(Class *cl)
{
    
    EnterFunc(bug("FreeClass(classID=%s)\n", cl->ClassID));
    
    if (cl)
    {
    	
    	/* What level are we ? */
    	if (cl->SuperClass == NULL) /* Someone trying to remove the rootclass */
    	    ReturnVoid("FreeClass (Someone trying to remove rootclass)");
    	
	FreeDispatchTables(cl);    
    	
    	/* Free class ID */
    	D(bug("Freeing class ID %s\n", cl->ClassID));
    	free(cl->ClassID);
    }
    ReturnVoid("FreeClass");
}

/************************
**  Rootclass methods  **
************************/
struct RootData
{
    ULONG dummy;
};

#define NUMROOTMETHODS 2

Object *_Root_New(Class *root_cl, Class *cl, APTR param)
{
    struct _Object *o;
    struct RootData *data;

    EnterFunc(bug("Root::New(cl=%s, param = %p)\n",
    	cl->ClassNode.ln_Name, param));
    
    /* Allocate memory for the object */
    D(bug("Object size: %ld\n", cl->InstOffset + cl->InstSize + sizeof (struct _Object)));
    o = malloc(cl->InstOffset + cl->InstSize + sizeof (struct _Object));
    if (o)
    {
    	D(bug("Mem allocated: %p\n", o));
    	o->o_Class = cl;
    	
    	data = (struct RootData *)BASEOBJECT(o);
    	
    	cl->ObjectCount ++;
    	
    	ReturnPtr ("Root::New", Object *, BASEOBJECT(o) );
    }
    
    ReturnPtr ("Root::New", Object *, NULL);
}

VOID _Root_Dispose(Class *cl, Object *o, Msg msg)
{
    EnterFunc(bug("Root::Dispose(o=%p, oclass=%s)\n", o, _OBJECT(o)->o_Class->ClassNode.ln_Name));

    _OBJECT(o)->o_Class->ObjectCount --;
    D(bug("Object mem: %p, size: %ld\n", _OBJECT(o), ((ULONG *)_OBJECT(o))[-1] ));
    free(_OBJECT(o));
    
    ReturnVoid("Root::Dispose");
}


/****************
**  InitOOP()  **
****************/


BOOL InitOOP()
{
#if (HASHED_STRINGS)
    struct MethodDescr mdescr[]=
    {
    	{ (IPTR (*)())_Root_New,		M_New		},
    	{ (IPTR (*)())_Root_Dispose,		M_Dispose	}
    };

    struct InterfaceDescr _Root_Descr[] =
    {
    	{mdescr, "Root", 2},
	{NULL, NULL, 0UL}
    };

#endif
#if (HASHED_IFS || HASHED_METHODS)
    IPTR (*_Root_MTab[])() =
    {
    	(IPTR (*)())_Root_New,
    	(IPTR (*)())_Root_Dispose
	
    };
    struct InterfaceDescr _Root_Descr[] =
    {
    	{_Root_MTab, I_Root, 2},
	{NULL, 0UL, 0UL},
    };
#endif

    Class *RootClass = &RootClassMem;
    
    InitSemaphore(&ClassListSema);
    NEWLIST(&(oopbase->ClassList));
    
    EnterFunc(bug("InitOOP()\n"));
    
    /* Initialize rootclass */
    RootClass->SuperClass = NULL;
    if (AllocDispatchTables(RootClass, _Root_Descr))
    {
    	RootClass->ClassID    	 = ROOTCLASS;
	RootClass->InstOffset 	 = 0UL;
	RootClass->InstSize   	 = 0UL;
	RootClass->SubClassCount = 0UL;
	RootClass->ObjectCount	 = 0UL;

#ifdef HASHED_IFS	
	RootClass->NumInterfaces = 1UL;
#endif	
	
	AddClass(RootClass);
	
	ReturnBool ("InitOOP", TRUE);
	
    }

     
    ReturnBool ("InitOOP", FALSE);
}


/*******************
**  CleanupOOP()  **
*******************/
VOID CleanupOOP()
{
    Class *RootClass = &RootClassMem;
    
    EnterFunc(bug("CleanupOOP()\n"));
    	
    FreeDispatchTables(RootClass);
    
    ReturnVoid("CleanupOOP");
}


/*****************
**  NewObject() **
*****************/
Object *NewObject(Class *cl, STRPTR classID, Msg msg)
{
    Object *o;

    struct P_New p;

    if (!cl)
    {
LockCL;
	cl = (Class *)FindName(&(oopbase->ClassList), classID);
	if (cl)
	   cl->ObjectCount ++; /* We don't want the class to be freed while we work on it */
    }
UnlockCL;

    if (!cl)
	return (NULL);

    /* Create a new instance */

    p.MethodID = (IPTR)M_New;
    p.ParamPtr = msg;

    o = (Object *)CoerceMethodA(cl, (Object *)cl, (Msg)&p);
    if (!o)
    {
	cl->ObjectCount --; /* Object creation failed, release lock */
    }
    return (o);
}

VOID DisposeObject(Object *o)
{
    IPTR methodid = (IPTR)M_Dispose;


    Class *cl = OCLASS(o);

    DoMethodA(o, (Msg)&methodid);

    cl->ObjectCount --;

    return;
}

/***************
**  AddClass  **
***************/
VOID AddClass(Class *cl)
{

LockCL;
    AddTail(&(oopbase->ClassList), &(cl->ClassNode) );
UnlockCL;
    return;
}

/******************
**  RemoveClass  **
******************/
VOID RemoveClass(Class *cl)
{
LockCL;
    Remove(&(cl->ClassNode) );
UnlockCL;

    return;
}
