/*
   (C) 1997-98 AROS - The Amiga Replacement OS
   $Id$

   Desc: 
   Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/lists.h>

#include <proto/oop.h>
#include <oop/oop.h>

#include "intern.h"
#include "hash.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


#define UB(x) ((UBYTE *)x)

#define ClassID PPart.ClassNode.ln_Name

/* This file contains support functions for allocating
  and initializing dispatch tables
*/

/* Hooks */
VOID FreeBucket(struct Bucket *b, struct IntOOPBase *OOPBase);
struct Bucket *CopyBucket(struct Bucket *old_b, APTR data, struct IntOOPBase *OOPBase);

/* Internal */
static struct IFBucket *CreateBucket(struct InterfaceDescr *ifDescr, struct IntOOPBase *OOPBase);

/*******************
**  CalcHTSize()  **
*******************/

/* Calculates the number of interfaces the new class has
   ( == number of buckets in the hashtable)
*/
static ULONG CalcHTEntries(struct IntClass *cl
		,struct InterfaceDescr *ifDescr
		,struct IntOOPBase *OOPBase)
{
    ULONG num_if = 0;
    struct IntClass *super = cl->SuperClass;
    
    EnterFunc(bug("CalcHTEntries(cl=%p, ifDescr=%p, super=%p)\n", cl, ifDescr, super));
  
 
    if (super)
    {
    	/* Get number of interfaces (method tables) in superclass */
    	num_if = super->NumInterfaces;
	D(bug("Super-interfaces: %ld\n", num_if));
    
    	/* Check if there are any new interfaces in this class */
    	for (; ifDescr->MethodTable; ifDescr ++)
    	{
	    D(bug("Checking for interface %s\n", ifDescr->InterfaceID));
	    
	    /* Look up in interface hashtable to see if it exists in superclass */
	    if ( NULL == super->IFTable->Lookup(super->IFTable, (IPTR)GetID(ifDescr->InterfaceID), OOPBase) )
	    {
	        D(bug("Found new interface: %s\n", ifDescr->InterfaceID));
		
		/* If it didn't then we have a new interface for this class */
	    	num_if ++;
	    }
	
    	} /* for (each interface in the description for the class) */
    
    }
    else
    {
    	/* This is a baseclass, count the interfaces */
    	for (; ifDescr->MethodTable; ifDescr ++)
    	{
	    num_if ++;
	
    	} /* for (each interface in the description for the class) */
    }
    ReturnInt ("CalcHTEntries", ULONG, num_if);
}



/****************************
**  AllocDispatchTables()  **
****************************/

/* Allocates and initializes the interface hashtable, and the methodtables */
BOOL AllocDispatchTables(
		struct IntClass 	*cl,
		struct InterfaceDescr 	*ifDescr,
		struct IntOOPBase 	*OOPBase)
{
    ULONG num_if;
    
    EnterFunc(bug("AllocDispatchTables(cl=%p,ifDescr=%p)\n",
    	cl, ifDescr));
    
    /* Get number of needed hash entries */
    num_if = CalcHTEntries(cl, ifDescr, OOPBase);
    cl->NumInterfaces = num_if;
    
    /* Create a new integer hashtable, with a reasonable size */
    cl->IFTable = NewHash(num_if, HT_INTEGER, OOPBase);
    if (cl->IFTable)
    {
    	/* Save hashmask for use in method lookup */
	cl->HashMask = HashMask(cl->IFTable);
	
	/* Copy parent interfaces into the new class */
	if (cl->SuperClass) /* This test makes it work for initializong rootclass */
	{
	    D(bug("Copying superclass' hashtable\n"));
	    if (!CopyHash(cl->IFTable
	    		,cl->SuperClass->IFTable
			,CopyBucket
			,(APTR)cl
			,OOPBase ))
	    {
	    	goto failure;
	    }	
	}
	
	/* Insert our own interfaces */
	D(bug("Inserting own methods\n"));
	for (; ifDescr->MethodTable; ifDescr ++)
	{
	    struct IFBucket *ifb;
	    ULONG i;
	    
	    /* Lookup hashtable to see if interface has been copied from superclass */
	    ifb = (struct IFBucket *)cl->IFTable->Lookup(cl->IFTable, (IPTR)ifDescr->InterfaceID, OOPBase);
	    if (!ifb)
	    {
	    	D(bug("Bucket doesn't exist, creating..\n"));
	    	/* Bucket doesn't exist, allocate it */
		ifb = CreateBucket(ifDescr, OOPBase);
	    }
	    if (!ifb)
	    	goto failure;
		
	    D(bug("Inserting bucket\n"));
	    InsertBucket(cl->IFTable, (struct Bucket *)ifb, OOPBase);
	    
	    D(bug("Inserting methods\n"));

	    /* Ovveride the superclass methods with our new ones */
	    for (i = 0; ifDescr->MethodTable[i].MethodFunc; i ++)
	    {
   	    	if (ifDescr->MethodTable[i].MethodFunc)
	    	{
	   	    ifb->MethodTable[ ifDescr->MethodTable[i].MethodIdx ].MethodFunc = ifDescr->MethodTable[i].MethodFunc;
	    	    ifb->MethodTable[ ifDescr->MethodTable[i].MethodIdx ].mClass     = (Class *)cl;
	    	}
	    } /* for (each method in the interface) */
	    
	} /* for (each interface to add to class) */
	
	/* For speedup in method lookup */
	cl->IFTableDirectPtr = (struct IFBucket **)cl->IFTable->Table;
	
    }
    ReturnBool ("AllocDispatchTables", TRUE);

failure:
    FreeHash(cl->IFTable, FreeBucket, OOPBase);
    ReturnBool ("AllocDispatchTables", FALSE);
}

/***************************
**  FreeDispatchTables()  **
***************************/
VOID FreeDispatchTables(struct IntClass *cl, struct IntOOPBase *OOPBase)
{
    /* This frees the hashtable + all buckets */
    FreeHash(cl->IFTable, FreeBucket, OOPBase);
    
    return;
}


/*********************
**  CreateBucket()  **
*********************/
/* Creates a new interface bucket */
static struct IFBucket *CreateBucket(
				struct InterfaceDescr 	*ifDescr,
				struct IntOOPBase 	*OOPBase)
{
    struct IFMethod *ifm = NULL;
    ULONG numentries = ifDescr->NumMethods;
    ULONG mtab_size = UB (&ifm[numentries]) - UB( &ifm[0]);
    
    /* Allocate bucket */
    struct IFBucket *ifb;
    
    ifb = (struct IFBucket *)AllocMem( sizeof (struct IFBucket), MEMF_ANY );
    if (ifb)
    {
    	/* Allocate method table for this interface */
    	ifb->MethodTable = (struct IFMethod *)AllocVec(mtab_size, MEMF_ANY);
	if (ifb->MethodTable)
	{
	    /* Get correct ID for the interface (string ID => interface ID mapping) */
	    ifb->InterfaceID = GetID(ifDescr->InterfaceID);
	    if (ifb->InterfaceID)
	    {
	    	/* Save number of methods in the interface */
	    	ifb->NumMethods  = ifDescr->NumMethods;
		return (ifb);

	    }
	}
	FreeMem (ifb, sizeof (struct IFBucket));
    }
    return (NULL);
}    
    

/***********************
**  Hash table hooks  **
***********************/
#define IB(x) ((struct IFBucket *)x)

VOID FreeBucket(struct Bucket *b, struct IntOOPBase *OOPBase)
{

    /* Free methodtable */
    FreeVec(IB(b)->MethodTable);

    /* Free the bucket itself */
    FreeMem(b, sizeof (struct IFBucket));
    

    return;
}

/* Copies a hashtable bucket */
struct Bucket *CopyBucket(struct Bucket *old_b, APTR data, struct IntOOPBase *OOPBase)
{
    struct IFBucket *new_b;
    
    EnterFunc(bug("CopyBucket(old_b=%p)\n", old_b));
    
    /* Allocate memory for the new interface bucket */
    new_b = (struct IFBucket *)AllocMem(sizeof (struct IFBucket), MEMF_ANY );
    if (new_b)
    {
    	struct IFMethod *ifm = NULL;
        ULONG mtab_size;
	
	/* Get number of methods in source methodtable */
	ULONG numentries = IB(old_b)->NumMethods;
	
	mtab_size = UB(&ifm[numentries]) - UB(&ifm[0]);
	
	/* Allocate memory for methodtable of same size as source one */
    	new_b->MethodTable = (struct IFMethod *)AllocVec(mtab_size, MEMF_ANY);
	if (new_b->MethodTable)
	{
	    /* Copy methodtable to destination */
	    CopyMem(IB(old_b)->MethodTable, new_b->MethodTable, mtab_size);
	    
    	    /* Initialize bucket */
	    new_b->InterfaceID  = IB(old_b)->InterfaceID;
	    new_b->NumMethods 	= IB(old_b)->NumMethods;
	    	    
	    ReturnPtr ("CopyBucket", struct Bucket *, (struct Bucket *)new_b );
	}
	FreeMem (new_b, sizeof (struct IFBucket));
    }
    
    ReturnPtr ("CopyBucket", struct Bucket *, NULL);
}


/***************
**  GetIDs()  **
***************/

/* Used to do several string interface ID to numeric interface ID mappings in one call */
BOOL GetIDs(struct IDDescr *idDescr, struct IntOOPBase *OOPBase)
{
    while (idDescr->ID)
    {
    	*(idDescr->Storage) = GetID(idDescr->ID);
	if (! *(idDescr->Storage))
	    return (FALSE);
	    
	idDescr ++;
    }
    return (TRUE);
}

