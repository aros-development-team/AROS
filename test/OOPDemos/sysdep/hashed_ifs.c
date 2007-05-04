/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "oop.h"
#include "sysdep/hashed_ifs.h"
#include "hash.h"
#include <stdlib.h>

#define SDEBUG 0
#define DEBUG 0
#include "debug.h"

#define UB(x) ((UBYTE *)x)

#define ClassID ClassNode.ln_Name
/* Hooks */
VOID FreeBucket(struct Bucket *b);
struct Bucket *CopyBucket(struct Bucket *old_b, APTR data);

/* Internal */
static struct InterfaceBucket *CreateBucket(struct InterfaceDescr *ifDescr);

/*******************
**  CalcHTSize()  **
*******************/

static ULONG CalcHTEntries(Class *cl, struct InterfaceDescr *ifDescr)
{
    ULONG num_if = 0;
    Class *super = cl->SuperClass;
    
    EnterFunc(bug("CalcHTEntries(cl=%s, ifDescr=%p)\n", cl->ClassID, ifDescr));
    
    /* Count the number of interfaces of superclass */
    
    if (super)
    {
    	num_if = super->NumInterfaces;
    
    	/* Check if there are any new interfaces in this class */
    	for (; ifDescr->MethodTable; ifDescr ++)
    	{
	    if ( NULL == HashLookupULONG((struct Bucket **)super->HashTable, ifDescr->InterfaceID) )
	    {
	    	num_if ++;
	    }
	
    	} /* for (each interface in the description for the class) */
    
    }
    else
    {
    	/* This is rootclass, count the interfaces */
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

BOOL AllocDispatchTables(Class *cl, struct InterfaceDescr *ifDescr)
{
    ULONG num_if;
    
    EnterFunc(bug("AllocDispatchTables(cl=%s,ifDescr=%p)\n",
    	cl->ClassID, ifDescr));
    
     /* Get hash table size */
    num_if = CalcHTEntries(cl, ifDescr);
    cl->NumInterfaces = num_if;
    
    cl->HashTable = (struct InterfaceBucket **)NewHash(num_if);
    if (cl->HashTable)
    {
	cl->HashMask = HashMask(cl->HashTable);
	
	/* Copy parent interfaces into the new class */
	if (cl->SuperClass) /* This test makes it work for initializong rootclass */
	{
	    if (!CopyHash((struct Bucket **)cl->HashTable
	    		,(struct Bucket **)cl->SuperClass->HashTable
			,CopyBucket
			,(APTR)cl) )
	    {
	    	goto failure;
	    }	
	}
	
	/* Insert our own interfaces */
	for (; ifDescr->MethodTable; ifDescr ++)
	{
	    struct InterfaceBucket *ifb;
	    ULONG i;
	    
	    
	    ifb = (struct InterfaceBucket *)HashLookupULONG((struct Bucket **)cl->HashTable, ifDescr->InterfaceID);
	    if (!ifb)
	    	/* Bucket doesn't exist, allocate it */
		ifb = CreateBucket(ifDescr);
		
	    if (!ifb)
	    	goto failure;
		
	    InsertBucket((struct Bucket **)cl->HashTable, (struct Bucket *)ifb);

	    for (i = 0; i < ifb->NumMethods; i ++)
	    {
   	    	if (ifDescr->MethodTable[i])
	    	{
	   	    ifb->MethodTable[i].MethodFunc = ifDescr->MethodTable[i];
	    	    ifb->MethodTable[i].mClass = cl;
	    	}
	    } /* for (each method in the interface) */
	    
	} /* for (each interface to add to class) */
    }
    ReturnBool ("AllocDispatchTables", TRUE);

failure:
    FreeHash((struct Bucket **)cl->HashTable, FreeBucket);
    ReturnBool ("AllocDispatchTables", FALSE);
}

/***************************
**  FreeDispatchTables()  **
***************************/
VOID FreeDispatchTables(Class *cl)
{
    FreeHash((struct Bucket **)cl->HashTable, FreeBucket);
    
    return;
}


/*********************
**  CreateBucket()  **
*********************/
static struct InterfaceBucket *CreateBucket(struct InterfaceDescr *ifDescr)
{
    struct IFMethod *ifm = NULL;
    ULONG numentries = ifDescr->NumMethods;
    ULONG mtab_size = UB (&ifm[numentries]) - UB( &ifm[0]);
    
    /* Allocate bucket */
    struct InterfaceBucket *ifb;
    
    ifb = (struct InterfaceBucket *)malloc( sizeof (struct InterfaceBucket) );
    if (ifb)
    {
    	ifb->MethodTable = (struct IFMethod *)malloc(mtab_size);
	if (ifb->MethodTable)
	{
	    ifb->InterfaceID = ifDescr->InterfaceID;
	    ifb->NumMethods      = ifDescr->NumMethods;
	    
	    return (ifb);
	}
	free (ifb);
    }
    return (NULL);
}    
    

/***********************
**  Hash table hooks  **
***********************/
#define IB(x) ((struct InterfaceBucket *)x)

VOID FreeBucket(struct Bucket *b)
{
//    D(bug("FreeBucket(b=%p)\n", b));
    free(IB(b)->MethodTable);
//    D(bug("Freeing bucket\n"));
    free(b);
    
//    ReturnVoid("FreeBucket");
    return;
}

struct Bucket *CopyBucket(struct Bucket *old_b, APTR data)
{
    struct InterfaceBucket *new_b;
    
    EnterFunc(bug("CopyBucket(old_b=%p)\n", old_b));
    
    new_b = (struct InterfaceBucket *)malloc(sizeof (struct InterfaceBucket) );
    if (new_b)
    {
    	struct IFMethod *ifm = NULL;
        ULONG mtab_size;
	ULONG numentries = IB(old_b)->NumMethods;
	
	mtab_size = UB(&ifm[numentries]) - UB(&ifm[0]);
	
    	new_b->MethodTable = (struct IFMethod *)malloc(mtab_size);
	if (new_b->MethodTable)
	{
	    /* Copy methodtable */
	    memcpy(new_b->MethodTable, IB(old_b)->MethodTable, mtab_size);
	    
    	    /* Initialize bucket */
	    new_b->InterfaceID  = IB(old_b)->InterfaceID;
	    new_b->NumMethods 	= IB(old_b)->NumMethods;
	    	    
	    ReturnPtr ("CopyBucket", struct Bucket *, (struct Bucket *)new_b );
	}
	free (new_b);
    }
    
    ReturnPtr ("CopyBucket", struct Bucket *, NULL);
}

