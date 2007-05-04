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
#include "intern.h"

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

static struct Bucket **AllocHash(Class *cl, ULONG numnewmethods);
static VOID FreeHash(struct Bucket **htable, ULONG htablesize);
static BOOL InitHash(Class *cl, struct MethodDescr *mDescr);

/****************
**  MakeClass  **
****************/
Class *MakeClass(STRPTR classID
		,STRPTR superID
		,struct MethodDescr *mDescr
		,ULONG instDataSize
		,ULONG numNewMethods)
{
    #define UB(x) ((UBYTE *)x)

    #define UnlockCL	 ReleaseSemaphore( &ClassListSema )
    #define LockCLShared ObtainSemaphoreShared( &ClassListSema )
    #define LockCL	 ObtainSemaphore( &ClassListSema )

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

	    /* Must be done before calling AllocHash().
	    ** This is because AllocHash() must know the number
	    ** of methods in this class and suoerclasses, so it can
	    ** allocate a hashtable of a sane size-
	    */
	    cl->SuperClass = supercl;
	    cl->NumMethods = numNewMethods;

	    D(bug("Number of methods introduced by class: %ld\n", numNewMethods));

	    /* Allocate interfaces for class */
	    if ( (cl->HashTable = AllocHash(cl, numNewMethods)) )
	    {
		/* Initialize hash table with methods */
		if (InitHash(cl, mDescr))
		{
		    /* Well, same as in BOOPSI */
		    cl->InstOffset = supercl->InstOffset + supercl->InstSize;
		    cl->InstSize   = instDataSize;

		    supercl->SubClassCount ++;

		    ReturnPtr ("MakeClass", Class *, cl);
		}
		FreeHash(cl->HashTable, cl->HashTableSize);

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
**  AllocHash()  **
******************/

/* Allocates and initializes the hashtable that is used
** to look up interface IDs.
** We always make sure that

*/
static ULONG NumHashEntries(ULONG initial)
{

    /* Calulates hashsize as 2^n - 1 so that htsize >= 2*initial */
    ULONG temp = 1;
    BYTE i;

    /* Find the highest bit in 'initial' */
    for (i = 31; i >= 0; i --)
    {
	if ((temp << i) & initial)
	    break;
    }

    /* Make sure table is never more than 50% full */
    i ++;

    return ((temp << i) - 1);

}

static struct Bucket **AllocHash(Class *cl, ULONG numNewMethods)
{

    LONG htable_size;

    struct Bucket **htable = NULL; /* keep compiler happy */
    ULONG nummethods, numentries;

    Class *super;

    D(bug("AllocHash(class=%s, numNewMethods=%ld)\n",
	cl->ClassID, numNewMethods));

    /* Count the number of methods for superclasses and their interfaces.
    ** Note that the same methods are NOT counted twice as
    ** class->NumMethods is the number of methods that are new for 'class'.
    */
    nummethods = numNewMethods;
    for (super = cl->SuperClass; super; super = super->SuperClass)
	nummethods += super->NumMethods;

    D(bug("Total number of methods: %ld\n", nummethods));

    /* Calculate hash table size, counted in entries */
    numentries = NumHashEntries(nummethods);

    /* Calculate hash table size, counted in bytes */
    htable_size = UB(&htable[numentries]) - UB(&htable[0]);

    D(bug("Hash table size: %ld\n", htable_size));

    /* Save hash table size, counted in entries */
    cl->HashTableSize = numentries;

    /* Allocate hash table */
    htable = malloc(htable_size);
    if (htable)
	memset(htable, 0, htable_size);

    ReturnPtr ("AllocHash", struct Bucket **, htable);
}

static BOOL AddMethods(Class *cl, struct Bucket **desthtable, ULONG htable_size)
{
    /* The class's methods into the supplied hashtable */

    ULONG i;

    D(bug("AddMethods(cl=%s, desthtable=%p, htable_size=%ld)\n",
	cl->ClassID, desthtable, htable_size));

    /* For each entry in the class' hashtable */
    for (i = 0; i < cl->HashTableSize; i ++)
    {
	struct Bucket *b;
	D(bug("Adding methods at idx %ld\n", i));

	/* For each bucket at the current entry */
	for (b = cl->HashTable[i]; b; b = b->Next)
	{
	    ULONG idx;
	    struct Bucket *new_b, *temp_b;

	    D(bug("Adding method %ld\n", b->MethodID));

	    /* Allocate new bucket into which the method is copied */
	    new_b = malloc( sizeof (struct Bucket) );
	    if (!new_b)
		ReturnBool ("AddMethods", FALSE);

	    /* Copuy methid info into new bucket */
	    new_b->MethodID   = b->MethodID;
	    new_b->MethodFunc = b->MethodFunc;
	    new_b->mClass     = b->mClass;


	    /* Add bucket to destination hashtable */
	    idx = CalcHash(b->MethodID, htable_size);

	    /* Adding it at the first position in the bucket linked list */
	    temp_b = desthtable[idx];
	    desthtable[idx] = new_b;
	    new_b->Next = temp_b;

	} /* for (each bucket at the current index) */

    } /* for (each index in the hashtable) */
    ReturnBool ("AddMethods", TRUE);
}

static BOOL InitHash(Class *cl, struct MethodDescr *mDescr)
{
    D(bug("InitHash(cl=%s, mDescr=%p\n", cl->ClassID, mDescr));
    if (cl->SuperClass) /* This test is so we can use this function to initalize ROOTCLASS */
    {
	D(bug("Superclass found: %s\n", cl->SuperClass->ClassID));

	/* Put all superclass' methods into our hash table */
	if (!AddMethods(cl->SuperClass, cl->HashTable, cl->HashTableSize))
	   ReturnBool ("InitHash", FALSE);
    }

    /* Override/insert the methods supplied in MakeClass() */
    D(bug("Ovverriding methods\n"));

    /* For each entry in the method description array supplied to MakeClass() */
    for (; mDescr->MethodFunc; mDescr ++)
    {
	ULONG idx;
	struct Bucket *b;
	BOOL must_allocate_new = TRUE;

	/* Look at which entry in the hdestination hashtable to put the ID */
	idx = CalcHash(mDescr->MethodID, cl->HashTableSize);

	/* Search for allready existing bucket containing the ID */
	for (b = cl->HashTable[idx]; b; b = b->Next)
	{
	    if (b->MethodID == mDescr->MethodID)
	    {

		/* The method existed in the superclass. Override it */
		b->MethodFunc = mDescr->MethodFunc;
		b->mClass = cl;

		must_allocate_new = FALSE;
		break;
	    }
	}

	if (must_allocate_new)
	{
	    struct Bucket *new_b, *temp_b;

	    /* A new method introduced by this class
	    ** (belonging to this class' interface)
	    */

	    /* Allocate bucket for the new mwthod */

	    new_b = malloc( sizeof (struct Bucket) );
	    if(!new_b)
		ReturnBool("InitHash", FALSE);

	    new_b->MethodID   = mDescr->MethodID;
	    new_b->MethodFunc = mDescr->MethodFunc;
	    new_b->mClass     = cl;

	    /* Insert the bucket at the start of the hash entry */
	    temp_b = cl->HashTable[idx];

	    cl->HashTable[idx] = new_b;

	    /* If there are no buckets in this table entry,
	    ** dest_b will be NULL, and new_b->Next becomes NULL
	    */
	    new_b->Next = temp_b;
	}

    } /* for (each method this class overrides) */
    ReturnBool ("InitHash", TRUE);
}


static VOID FreeHash(struct Bucket **htable, ULONG htablesize)
{
    ULONG i;

    /* Well, frees a hashtable + the buckets */
    for (i = 0; i < htablesize; i ++)
    {
	struct Bucket *b, *next_b;

	b = htable[i];
	while (b)
	{
	    next_b = b->Next;
	    free(b);
	    b = next_b;
	}

    }
    free(htable);
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

	FreeHash(cl->HashTable, cl->HashTableSize);
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
	o->oClass = cl;

	cl->ObjectCount ++;

	ReturnPtr ("Root::New", Object *, BASEOBJECT(o) );
    }

    ReturnPtr ("Root::New", Object *, NULL);
}

VOID Root_Dispose(Class *cl, Object *o, Msg msg)
{
    D(bug("Root::Dispose(o=%p, oclass=%s)\n", o, _OBJECT(o)->oClass->ClassNode.ln_Name));

    _OBJECT(o)->oClass->ObjectCount --;
    D(bug("Object mem: %p\n", _OBJECT(o) ));
    free(_OBJECT(o));

    ReturnVoid("Root::Dispose");
}


/****************
**  InitOOP()  **
****************/

#define NUM_ROOT_METHODS 2


BOOL InitOOP()
{
    Class *RootClass = &RootClassMem;
    struct MethodDescr RootMethodDescr[] =
    {
	{ Root_New,	M_New },
	{ Root_Dispose, M_Dispose},
	{ NULL, 0 }
    };


    InitSemaphore(&ClassListSema);
    NEWLIST(&ClassList);


    /* We must initialize the rootclass by hand */

    RootClass->SuperClass = NULL; /* !!! Very important to do before AllocHash() !!! */
    RootClass->HashTable = AllocHash(RootClass, NUM_ROOT_METHODS);
    if (RootClass->HashTable)
    {
	if (InitHash(RootClass, RootMethodDescr))
	{
	    /* Fill in other stuff into the class structure */
	    RootClass->ClassNode.ln_Name = ROOTCLASS;
	    RootClass->InstOffset	 = 0UL;
	    RootClass->InstSize 	 = sizeof (struct RootData);
	    RootClass->NumMethods	 = NUMROOTMETHODS;
	    RootClass->SubClassCount	 = 0UL;
	    RootClass->ObjectCount	 = 0UL;
	    RootClass->SuperClass	 = NULL;

	    /* Add the class. Arbitration not necessary, as
	    ** noone know about us yet
	    */
	    AddTail(&ClassList, &(RootClass->ClassNode) );

	    return (TRUE);

	}
	FreeHash(RootClass->HashTable, RootClass->HashTableSize);

    }

    return (FALSE);
}


/*******************
**  CleanupOOP()  **
*******************/
VOID CleanupOOP()
{

    Class *RootClass = &RootClassMem;

    D(bug("CleanupOOP()\n"));

    FreeHash(RootClass->HashTable, RootClass->HashTableSize);

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
