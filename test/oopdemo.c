/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Demo of new OOP system
 */

/* Prevent inclusion of <intuition/classes.h>,
 * which is referenced in the amiga inline macros
 * imported below by <proto/exec.h>.
 */
#define INTUITION_CLASSES_H

#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>

#include <string.h>
#include <stdio.h>

#include "class.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


#define ENABLE_RT 1
#include <aros/rt.h>




/**************** String class definitions ***********************/

/* Subclass of root */
#define CL_Level_String (CL_Level_Root + 1)
#define IF_Level_String (IF_Level_Root + 1)

#define STRINGCLASSNAME "stringclass"

/* Public New() parameter struct */
typedef struct NewStringParam
{
     STRPTR InitStr;
     ULONG  MaxStrLen;
} P_String;

typedef struct StringInterface
{
    VOID (*SetStr)(Object, STRPTR);
    VOID (*PrintStr)(Object);
} IString;

typedef struct StringIFStorage
{
    IRoot	*IRoot;
    IString	*IString;

} IStringTable;


/* Private instance data definition */
struct StringData
{
    STRPTR Buffer;
    ULONG MaxStrLen;
};

/**************************************************/
VOID CleanupOOP();
BOOL InitOOP();
Object NewInstance(STRPTR classID, APTR, APTR *interfaceStorage);
Class *CreateClass(STRPTR classID, STRPTR superID, struct MTabDescr *mTabDescr,
	ULONG instDataSize);
VOID DeleteClass(STRPTR classID);


/* could go into a library base */
struct SignalSemaphore ClassListSema;
struct List ClassList;
Class RootClassMem;

/*************************** String ***************************/


/* Method implementations */

Object String_New(Class *cl, P_String *p)
{
    ULONG max = 50;
    STRPTR str = "";
    /* Get parent interface */
    IRoot *ir = (IRoot *)CL_INTERFACE(cl, CL_Level_Root,  IF_Level_Root);
    Object o;

    /* Whether it accepts NULL initparameter is class dependant.
    ** The parameter could very well be a taglist
    */
    if (p)
    {
    	if (p->InitStr)
    	    str = p->InitStr;
    	if (p->MaxStrLen)
    	    max = p->MaxStrLen;
    }

    o = ir->New(cl, NULL);
    if (o)
    {
    	STRPTR buffer;

    	/* Get instance data */
    	struct StringData *data = INST_DATA(o, CL_Level_String);

    	data->Buffer  = NULL;
    	data->MaxStrLen = 0;

    	buffer = AllocVec(max + 1, MEMF_ANY);
    	if (buffer)
    	{

    	    strncpy(buffer, str, max);

    	    data->Buffer = buffer;
    	    data->MaxStrLen = max;

    	    return (o);

    	}

    	/* Do a "CoerceMethod" (I get the Root interface from the *String* class) */
    	ir = (IRoot *)CL_INTERFACE(cl, CL_Level_String, IF_Level_Root);
    	ir->Dispose(o);
    }
    return (NULL);
}

VOID String_Dispose(Object o)
{
    struct StringData *data = INST_DATA(o, CL_Level_String);

    IRoot *ir = (IRoot *)OBJ_INTERFACE(o, CL_Level_Root, IF_Level_Root);


    if (data->Buffer)
    {
    	FreeVec(data->Buffer);
    }

    ir->Dispose(o); /* Call supermethod */

    return;
}

VOID String_SetStr(Object o, STRPTR str)
{
    struct StringData *data = INST_DATA(o, CL_Level_String);


    strncpy(data->Buffer, str, data->MaxStrLen);

    return;
};

VOID String_PrintStr(Object o)
{
    struct StringData *data;

    data = INST_DATA(o, CL_Level_String);

    printf("%s", data->Buffer);

    return;
}

/* Method tables */
IRoot str_iroot = { (Object (*)(Class *, APTR))String_New, String_Dispose};

IString str_istring = {String_SetStr, String_PrintStr};

struct MTabDescr str_descr[] =
{
    {(APTR *)&str_iroot,	2}, /* Table contains two entries */
    {(APTR *)&str_istring,	2}  /* Table contains two entries */
};


int main(int argc, char **argv)
{

    SDInit();

    RT_Init();

    if (InitOOP())
    {
	struct Node *n;
	Object o;


    	printf("Object system initialized\n");

    	/* Initialize the string class */
    	if (CreateClass(STRINGCLASSNAME, ROOTCLASSNAME, str_descr, sizeof (struct StringData)) )
    	{
	    IStringTable st;
	    P_String pstr = {"String test", 80};

    	    printf("Class list:\n");
    	    ForeachNode(&ClassList, n)
    	    {
    	    	printf("%s\n", n->ln_Name);
    	    }
    	    printf("\n\n");

    	    /* Create a new instance */
    	    o = NewInstance(STRINGCLASSNAME, &pstr, (APTR *)&st);
    	    if (o)
    	    {
    	    	IString *is = st.IString;

    	    	printf("New instance: %p\n", o);

    	    	printf("Current string:  ");
    	    	is->PrintStr(o);
    	    	printf("\n");

    	    	printf("Setting string\n");
    	    	is->SetStr(o, "Blah");

    	    	printf("New string value:  ");
    	    	is->PrintStr(o);
    	    	printf("\n");

    	    	st.IRoot->Dispose(o);

    	    	printf("object deleted\n");
    	    }
    	    DeleteClass(STRINGCLASSNAME);
    	}
    	CleanupOOP();
    }

    RT_Exit();

    return (0);
}




/******************************************************************/

/* The oop system code */

BOOL AllocInterfaces(Class *cl);
VOID FreeInterfaces(Class *cl);




/******************
**  CreateClass  **
******************/
Class *CreateClass(STRPTR classID, STRPTR superID, struct MTabDescr *mTabDescr,
	ULONG instDataSize)
{
    #define UB(x) ((UBYTE *)x)

    #define UnlockCL   	 ReleaseSemaphore( &ClassListSema )
    #define LockCLShared ObtainSemaphoreShared( &ClassListSema )
    #define LockCL 	 ObtainSemaphore( &ClassListSema )

    #define ClassID ClassNode.ln_Name

    Class *cl, *supercl;
    UWORD super_count;

    /* Find superclass */
    EnterFunc(bug("CreateClass(classID=%s, superID=%s, mTabDescr=%p, instDataSize=%d)\n",
    		classID, superID, mTabDescr, instDataSize));

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
    cl = AllocMem(sizeof (Class), MEMF_PUBLIC|MEMF_CLEAR); /* Can be made read-only for MP */
    if (cl)
    {
    	ULONG cltab_size, iftab_size;

    	Class *superptr;

    	D(bug("Allocated class structure\n"));

	/* Find this class's level in the hierarchy */

    	/* How many super-classes are there ? */
    	super_count = 1; /* have allready found one superclass */
    	for (superptr = supercl; superptr->SuperClass; superptr = superptr->SuperClass)
    	    super_count ++;

    	D(bug("Number of superclasses: %d\n", super_count));

        /* Get size of interface table */
    	iftab_size = UB(&(cl->InterfaceTable[super_count + 1])) - UB(&(cl->InterfaceTable[0]));
    	D(bug("IF tabsize: %d\n", iftab_size));

    	/* Allocate interface table pointers */
    	cl->InterfaceTable = AllocMem(iftab_size, MEMF_PUBLIC|MEMF_CLEAR); /* Can be made read-only for MP */

        /* Get size of class table */
    	cltab_size = UB( &(cl->ClassTable[super_count + 1])) - UB( &(cl->ClassTable[0]));
    	D(bug("CL tabsize: %d\n", cltab_size));

    	/* Allocate interface methodtable pointers */
    	cl->ClassTable = AllocMem(cltab_size, MEMF_PUBLIC|MEMF_CLEAR); /* Can be made read-only for MP */
    	D(bug("Classtable allocated: %p\n"));

    	cl->ClassID = AllocVec( strlen(classID) + 1, MEMF_PUBLIC);

    	if (cl->InterfaceTable && cl->ClassTable && cl->ClassID)
    	{

    	    D(bug("Tables allocated\n"));
    	    /* Copy class ID */
    	    strcpy(cl->ClassID, classID);
    	    D(bug("class ID copied\n"));

	    /* !!! Important to fill in these before calling AllocInterfaces */
    	    cl->NumMethods = mTabDescr[super_count].NumMethods;
    	    cl->SuperClass = supercl;

    	    D(bug("Number of methods introduced by class: %d\n", cl->NumMethods));

    	    /* Allocate interfaces for class */
    	    if (AllocInterfaces(cl))
    	    {
    	    	IPTR *iftab;
    	    	APTR *mtab;
    	    	WORD idx, m_idx;
    	    	Class *superptr = supercl;
    	    	IPTR *ifptr;

    	    	/* Fill in the interfaces from the superclass */
    	    	for (idx = super_count - 1; idx >= 0; idx -- )
    	    	{
    	    	    D(bug("Getting interface %d with %d methods from class %s\n",
    	    	    	idx, superptr->NumMethods, superptr->ClassID));

    	    	    CopyMem(	superptr->InterfaceTable[idx],
    	    	    	cl->InterfaceTable[idx],
    	    	    	UB( &ifptr[superptr->NumMethods]) - UB( &ifptr[0]) );

    	    	    /* And fill in the class table while we're at it */
    	    	    cl->ClassTable[idx] = superptr;
    	    	    superptr = superptr->SuperClass;

    	    	}

    	    	/* Fill in ourselves in the classtable */
    	    	cl->ClassTable[super_count] = cl;

    	    	/* Override the interfaces with our own methods */
    	    	for (idx = super_count - 1; idx >= 0; idx -- )
    	    	{
    	    	    mtab  = mTabDescr[idx].Table;
    	    	    iftab = cl->InterfaceTable[idx];

    	    	    for (m_idx = mTabDescr[idx].NumMethods - 1; m_idx >= 0; m_idx -- )
    	    	    {
    	    	    	if (mtab[m_idx] != NULL)
    	    	    	{
    	    	    	    D(bug("Overriding method at cl_idx=%d, method=%d\n", idx, m_idx));
    	    	    	    D(bug("Method overrided: %p, overrider: %p\n", iftab[m_idx], mtab[m_idx]));
 			    /* Override method */
 			    iftab[m_idx] = (IPTR)mtab[m_idx];
    	    	    	}

    	    	    } /* For each method in the method table */

    	    	} /* For each level above us in the hierarchy */

    	    	/* Fill in the methods that are new with this class */
    	    	D(bug("Filling in methods at level %d\n", super_count));
    	    	CopyMem(mTabDescr[super_count].Table,
    	    	    cl->InterfaceTable[super_count],
    	    	    UB( &ifptr[ mTabDescr[super_count].NumMethods ]) - UB( &ifptr[0]) );


    	    	/* Update instance data info */
    	    	cl->InstOffset = supercl->InstOffset + supercl->InstSize;
    	    	cl->InstSize = instDataSize;

    	    	cl->SuperCount = super_count;

LockCL;
		AddTail(&ClassList, &(cl->ClassNode) );
UnlockCL;

    	    	ReturnPtr ("CreateClass", Class *, cl);

    	    }

    	    if (cl->InterfaceTable)
    	    	FreeMem(cl->InterfaceTable, iftab_size);

    	    if (cl->ClassTable)
    	    	FreeMem(cl->ClassTable, cltab_size);

    	    if (cl->ClassID)
    	    	FreeVec(cl->ClassID);

    	}

    	FreeMem(cl, sizeof (struct IClass));

    }

    /* Initalization failed, free lock on superclass */
    supercl->SubClassCount --;

    ReturnPtr ("CreateClass", Class *, NULL);

} /* CreateClass */



/************************
**  AllocInterfaces()  **
************************/
BOOL AllocInterfaces(Class *param_cl)
{
    IPTR **ifptr = param_cl->InterfaceTable;
    Class *cl;

    EnterFunc(bug("AllocInterfaces(cl=%s)\n", param_cl->ClassID));

    /* Allocate the interfaces */
    for ( cl = param_cl ; cl; cl = cl->SuperClass )
    {
    	ULONG size = UB( &((*ifptr)[cl->NumMethods])) - UB( &((*ifptr)[0]));

    	D(bug("Allocating interface for class %s, size=%d\n", cl->ClassID, size));
    	*ifptr = AllocMem(size, MEMF_PUBLIC|MEMF_CLEAR); /* Should be read-only for MP */
    	D(bug("IF Allocated: %p\n", *ifptr));
    	if (!*ifptr)
    	{
    	    FreeInterfaces(param_cl);
    	    ReturnBool ("AllocInterfaces", FALSE);
    	}

    	ifptr ++;

    }
    ReturnBool ("AllocInterfaces", TRUE);
}

/***********************
**  FreeInterfaces()  **
***********************/
VOID FreeInterfaces(Class *cl)
{
    IPTR **ifptr = cl->InterfaceTable;

    EnterFunc(bug("FreeInterfaces(cl=%s)\n", cl->ClassID));

    for ( ; cl; cl = cl->SuperClass )
    {
    	if (*ifptr)
    	{
    	    ULONG size = UB( &(*ifptr)[cl->NumMethods]) - UB(&(*ifptr)[0]);

    	    D(bug("Freeing IF %p of size %d\n", *ifptr, size));
    	    FreeMem(*ifptr, size);

	}
    	ifptr ++;
    }
    ReturnVoid ("FreeInterfaces");
}



/******************
**  DeleteClass  **
******************/

VOID DeleteClass(STRPTR classID)
{
    Class *cl;

    EnterFunc(bug("DeleteClass(classID=%s)\n", classID));

LockCLShared;
    cl = (Class *)FindName( &ClassList, classID);
UnlockCL;

    if (cl)
    {
    	ULONG size;
    	ULONG tab_entries = cl->SuperCount + 1;


    	/* What level are we ? */
    	if (cl->SuperCount == 0) /* Someone trying to remove the rootclass */
    	    ReturnVoid("DeleteClass (Someone trying to remove rootclass)");

    	/* Free interfaces */
    	FreeInterfaces(cl);

    	size = UB(&cl->InterfaceTable[tab_entries]) - UB(&cl->InterfaceTable[0]);
    	D(bug("Freeing IFTab of size %d\n", size));
    	FreeMem(cl->InterfaceTable, size);

    	size = UB(&cl->ClassTable[tab_entries]) - UB(&cl->ClassTable[0]);
    	D(bug("Freeing ClassTab of size %d at %p\n", size, cl->ClassTable));
    	FreeMem(cl->ClassTable, size);

    	/* Free class ID */
    	D(bug("Freeing class ID %s\n", cl->ClassID));
    	FreeVec(cl->ClassID);
LockCL;
		Remove(&(cl->ClassNode));
UnlockCL;

	FreeMem(cl, sizeof (Class));
    }
    ReturnVoid("DeleteClass");
}

/************************
**  Rootclass methods  **
************************/
struct RootData
{
    ULONG dummy;
};

#define NUMROOTMETHODS 2

Object Root_New(Class *cl, APTR param)
{
    struct _Object *o;
    struct RootData *data;

    EnterFunc(bug("Root::New(cl=%s, param = %p)\n",
    	cl->ClassNode.ln_Name, param));

    /* Allocate memory for the object */
    D(bug("Object size: %d\n", cl->InstOffset + cl->InstSize + sizeof (struct _Object)));
    o = AllocVec(cl->InstOffset + cl->InstSize + sizeof (struct _Object), MEMF_ANY);
    if (o)
    {
    	D(bug("Mem allocated: %p\n", o));
    	o->Class = cl;

    	data = (struct RootData *)BASEOBJECT(o);

    	cl->ObjectCount ++;

    	ReturnPtr ("Root::New", Object, BASEOBJECT(o) );
    }

    ReturnPtr ("Root::New", Object, NULL);
}

VOID Root_Dispose(Object o)
{
    EnterFunc(bug("Root::Dispose(o=%p, oclass=%s)\n", o, _OBJECT(o)->Class->ClassNode.ln_Name));

    _OBJECT(o)->Class->ObjectCount --;
    D(bug("Object mem: %p, size: %d\n", _OBJECT(o), ((ULONG *)_OBJECT(o))[-1] ));
    FreeVec(_OBJECT(o));

    ReturnVoid("Root::Dispose");
}


/****************
**  InitOOP()  **
****************/


BOOL InitOOP()
{
    Class *RootClass = &RootClassMem;

    InitSemaphore(&ClassListSema);
    NEWLIST(&ClassList);

    /* We must initialize the rootclass by hand */

    /* Class table. Only needs place for myself */
    RootClass->ClassTable = (struct IClass **)AllocMem( sizeof(Class *), MEMF_PUBLIC);
    if (RootClass->ClassTable)
    {

    	/* Interface table. Only needs place for our own interface */
    	RootClass->InterfaceTable = (IPTR **)AllocMem( sizeof (IPTR *), MEMF_PUBLIC);
    	if (RootClass->InterfaceTable)
    	{
    	    IPTR *interface;
    	    ULONG if_size;

    	    /* Allocate interface */
    	    if_size = UB(&interface[NUMROOTMETHODS]) - UB(&interface[0]);
    	    interface = AllocMem(if_size, MEMF_PUBLIC);
    	    if (interface)
    	    {
    	        /* Fill in methods */
    	        interface[0] = (IPTR)Root_New;
    	        interface[1] = (IPTR)Root_Dispose;

    	        RootClass->InterfaceTable[0] = interface;

    	        /* Fill in other stuff into the class structure */
    	        RootClass->ClassNode.ln_Name = ROOTCLASSNAME;
    	    	RootClass->ClassTable[0]  = RootClass;
    	    	RootClass->InstOffset 	  = 0UL;
    	    	RootClass->InstSize	  = sizeof (struct RootData);
    	    	RootClass->NumMethods	  = NUMROOTMETHODS;
    	    	RootClass->SubClassCount  = 0UL;
    	    	RootClass->ObjectCount	  = 0UL;
    	    	RootClass->SuperClass	  = NULL;
    	    	RootClass->SuperCount 	  = 0UL;

    	    	/* Add the class. Arbitration not necessary, as
    	    	** noone know about us yet
    	    	*/
    	    	AddTail(&ClassList, &(RootClass->ClassNode) );

    	    	return (TRUE);
    	    }

    	    FreeMem(RootClass->InterfaceTable, sizeof (IPTR **));
    	}
    	FreeMem(RootClass->ClassTable, sizeof (Class *));

    }

    return (FALSE);
}


/*******************
**  CleanupOOP()  **
*******************/
VOID CleanupOOP()
{
    IPTR *interface;
    ULONG if_size;

    Class *RootClass = &RootClassMem;

    FreeMem(RootClass->ClassTable, sizeof (APTR *));

    interface = RootClass->InterfaceTable[0];
    if_size = UB(&interface[NUMROOTMETHODS]) - UB(&interface[0]);

    FreeMem(interface, if_size);

    FreeMem(RootClass->InterfaceTable, sizeof (IPTR **));

    return;
}

/*******************
**  NewInstance() **
*******************/
Object NewInstance(STRPTR classID, APTR param, APTR *interfaceStorage)
{
    Class *cl;
    LONG idx;

    Object o;

    EnterFunc(bug("NewInstance(classID=%s, param=%p, ifStorage=%p)\n",
    	classID, param, interfaceStorage));

LockCL;
    cl = (Class *)FindName(&ClassList, classID);
    if (cl)
    	cl->ObjectCount ++; /* We don't want the class to be freed while we work on it */
UnlockCL;

    if (!cl)
    	return (NULL);

    /* Get interfaces from class */

    for ( idx = cl->SuperCount; idx >= 0; idx -- )
    	interfaceStorage[idx] = cl->InterfaceTable[idx];

    /* Create a new instance */
    D(bug("Calling New() at %p\n", ((IRoot *) interfaceStorage[cl->SuperCount])->New));
    o = ((IRoot *) interfaceStorage[0])->New(cl, param);

    if (!o)
    {
LockCL;
    	cl->ObjectCount --; /* Object creation failed, release lock */
UnlockCL;
    }
    ReturnPtr ("NewInstance", Object, o);
}
