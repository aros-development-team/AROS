/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/


/* These are not necessary in the demo */
#define InitSemaphore(x)
#define ObtainSemaphore(x) 
#define ObtainSemaphoreShared(x)
#define ReleaseSemaphore(x)

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "oop.h"
#include "protos.h"

#define DEBUG 0
#include "debug.h"

#define UB(x) ((UBYTE *)x)
/* could go into a library base */
/*struct SignalSemaphore ClassListSema;
*/
struct List ClassList;
Class RootClassMem;

/* Method implementations */


/******************************************************************/

/* The oop system code */

static BOOL AllocMTable(Class *cl, ULONG numNewMethods);
static VOID InitMTable(Class *cl, APTR *mDescr);
static VOID FreeMTable(Class *cl);

/****************
**  MakeClass  **
****************/
Class *MakeClass(STRPTR classID
		,STRPTR superID
		,APTR  *mDescr
		,ULONG instDataSize
		,ULONG numNewMethods)
{
    #define UB(x) ((UBYTE *)x)
    
    #define UnlockCL   	 ReleaseSemaphore( &ClassListSema )
    #define LockCLShared ObtainSemaphoreShared( &ClassListSema )
    #define LockCL 	 ObtainSemaphore( &ClassListSema )
    
    #define ClassID ClassNode.ln_Name

    Class *cl, *supercl;

    /* Find superclass */    
    D(bug("CreateClass(classID=%s, superID=%s, mDescr=%p, instDataSize=%ld)\n",
    		classID, superID, mDescr, instDataSize));

LockCL;
    supercl = (Class *)FindName( &ClassList, superID);
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
    	
    	cl->ClassID = malloc( strlen(classID) + 1 );
	
	if (cl->ClassID)
    	
    	{
    	    /* Copy class ID */
    	    strcpy(cl->ClassID, classID);
    	    D(bug("class ID copied\n"));

	    /* Must be done before calling AllocMethodTable().
	    ** This is because AllocMethodTable() must know the number
	    ** of new methods.
	    */
    	    cl->SuperClass = supercl;
	    cl->NumMethods = numNewMethods;

    	    D(bug("Number of methods introduced by class: %ld\n", numNewMethods));
    	    
    	    /* Allocate method table for class */
    	    if ( AllocMTable(cl, numNewMethods) )
    	    {
	        /* Initialize method table with supplied methods */
	    	InitMTable(cl, mDescr);
		    
		/* Well, same as in BOOPSI */
		cl->InstOffset = supercl->InstOffset + supercl->InstSize;
		cl->InstSize   = instDataSize;
		    
		supercl->SubClassCount ++;
		    
	    	ReturnPtr ("MakeClass", Class *, cl);

		
	    }
	    
	    free(cl->ClassID);
    	}
    	free(cl);
    }
    
    /* Initalization failed, free lock on superclass */
    supercl->SubClassCount --;
    
    ReturnPtr ("MakeClass", Class *, NULL);

} /* CreateClass */


/*************************
**  AllocMethodTable()  **
*************************/

static BOOL AllocMTable(Class *cl, ULONG numNewMethods)
{

    LONG mtable_size;

    ULONG nummethods;
    struct Method *mtable = NULL;
    
    Class *super;
    
    D(bug("AllocMTable(class=%s, numNewMethods=%ld)\n",
    	cl->ClassID, numNewMethods));
    
    /* Count the number of methods for superclasses.
    ** Note that the same methods are NOT counted twice as
    ** class->NumMethods is the number of methods that are new for 'class'.
    */
    nummethods = numNewMethods;
    for (super = cl->SuperClass; super; super = super->SuperClass)
    	nummethods += super->NumMethods;
	
    D(bug("Total number of methods: %ld\n", nummethods));
	
    /* Calculate method table size, counted in bytes */
    mtable_size = UB(&mtable[nummethods]) - UB(&mtable[0]);

    D(bug("Method table size: %ld\n", mtable_size));

    /* Save hash table size, counted in entries */
    cl->MTableSize = nummethods;
    
    /* Allocate hash table */
    mtable = malloc(mtable_size);
    if (mtable)
    {
    	memset(mtable, 0, mtable_size);
	cl->MTable = mtable;
	ReturnBool("AllocMTable", TRUE);
    }	
   
    
    ReturnBool ("AllocMTable", FALSE);
}


static VOID InitMTable(Class *cl, APTR *mDescr)
{

    Class *super = cl->SuperClass;
    ULONG i;
    
    D(bug("InitMTable(cl=%s, mDescr=%p\n", cl->ClassID, mDescr));
    
    if (super) 
    {
	ULONG super_mtable_size;
	
        D(bug("Superclass found: %s\n", super->ClassID));
	super_mtable_size =   UB(&(super->MTable[super->MTableSize]))
			    - UB(&(super->MTable[0]));
			    
	memcpy(cl->MTable, super->MTable, super_mtable_size);
    }
    
    /* Override using the methods supplied in MakeClass() */
    D(bug("Overriding methods\n"));
    
    for (i = 0; i < cl->MTableSize; i ++)
    {
	if (mDescr[i] != NULL)
	{

    	    D(bug("Overriding method %ld\n", i));
	    cl->MTable[i].MethodFunc = mDescr[i];
	    cl->MTable[i].m_Class      = cl;
	}
    }
    ReturnVoid("InitMTable");
}


static VOID FreeMTable(Class *cl)
{
    free(cl->MTable);
    return;
}

/*****************
**  Freelass()  **
*****************/
VOID FreeClass(Class *cl)
{
    
    D(bug("FreeClass(cl=%s)\n", cl->ClassID));

    
    if (cl)
    {
    	/* What level are we ? */
    	if (cl->SuperClass == NULL) /* Someone trying to remove the rootclass */
    	    ReturnVoid("FreeClass (Someone trying to remove rootclass)");
	 
	FreeMTable(cl);
    	free(cl->ClassID);
	free(cl);
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

Object *Root_New(Class *rootcl, Class *cl, Msg msg)
{
    struct _Object *o;

    D(bug("Root::New(cl=%s, msg = %p)\n",
    	cl->ClassNode.ln_Name, msg));
    
    /* Allocate memory for the object */
    D(bug("Object size: %ld\n", cl->InstOffset + cl->InstSize + sizeof (struct _Object)));
    o = malloc(cl->InstOffset + cl->InstSize + sizeof (struct _Object) );
    if (o)
    {
    	D(bug("Mem allocated: %p\n", o));
    	o->o_Class = cl;
    	
    	cl->ObjectCount ++;
    	
    	ReturnPtr ("Root::New", Object *, BASEOBJECT(o) );
    }
    
    ReturnPtr ("Root::New", Object *, NULL);
}

VOID Root_Dispose(Class *cl, Object *o, Msg msg)
{
    D(bug("Root::Dispose(o=%p, oclass=%s)\n", o, _OBJECT(o)->o_Class->ClassNode.ln_Name));

    _OBJECT(o)->o_Class->ObjectCount --;
    D(bug("Object mem: %p\n", _OBJECT(o) ));
    free(_OBJECT(o));
    
    ReturnVoid("Root::Dispose");
}


/****************
**  InitOOP()  **
****************/



BOOL InitOOP()
{
    Class *RootClass = &RootClassMem;
    APTR RootMTable[] =
    {
    	Root_New,
	Root_Dispose,
	NULL
    };
    
    D(bug("InitOOP()\n"));
    
    InitSemaphore(&ClassListSema);
    NEWLIST(&ClassList);

    
    /* We must initialize the rootclass by hand */
    
    RootClass->SuperClass = NULL; /* !!! Very important to do before AllocMTable() !!! */
    if (AllocMTable(RootClass, NUM_ROOT_METHODS))
    {
        D(bug("MTable allocated\n"));
    	InitMTable(RootClass, RootMTable);

    	/* Fill in other stuff into the class structure */
    	RootClass->ClassNode.ln_Name = ROOTCLASS;
    	RootClass->InstOffset 	 = 0UL;
    	RootClass->InstSize	 = sizeof (struct RootData);
    	RootClass->NumMethods	 = NUMROOTMETHODS;
    	RootClass->SubClassCount = 0UL;
    	RootClass->ObjectCount	 = 0UL;
    	RootClass->SuperClass	 = NULL;
    	    	
    	/* Add the class. Arbitration not necessary, as
    	** noone know about us yet
    	*/
    	AddTail(&ClassList, &(RootClass->ClassNode) );
    	    	
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
    
    D(bug("CleanupOOP()\n"));
    	
    FreeMTable(RootClass);
    	
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
    	cl = (Class *)FindName(&ClassList, classID);
    	if (cl)
    	   cl->ObjectCount ++; /* We don't want the class to be freed while we work on it */
    }	   
UnlockCL;

    if (!cl)
    	return (NULL);
    	
    /* Create a new instance */
    
    p.MethodID = M_New;
    p.ParamPtr = msg;
    
    o = (Object *)CoerceMethodA(cl, (Object *)cl, (Msg)&p);
    if (!o)
    {
    	cl->ObjectCount --; /* Object creation failed, release lock */
    }
    ReturnPtr ("NewInstance", Object *, o);
}

VOID DisposeObject(Object *o)
{
    ULONG methodid = M_Dispose;
    
    
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
    AddTail(&ClassList, &(cl->ClassNode) );
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
