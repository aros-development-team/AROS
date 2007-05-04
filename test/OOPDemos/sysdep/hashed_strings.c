/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system.
   Lang: english
*/

#include "oop.h"
#include "hash.h"
#include "sysdep/hashed_strings.h"
#include <stdlib.h>

#define SDEBUG 0
#define DEBUG 0
#include "debug.h"

#define ClassID ClassNode.ln_Name

static struct Bucket *CopyBucket(struct Bucket *old_b);
static VOID FreeBucket(struct Bucket *b);

/**********************
**  NumNewMethods()  **
**********************/

static ULONG NumNewMethods(Class *cl, struct InterfaceDescr *ifDescr)
{
    struct Bucket **ht;
    ULONG numnewmethods = 0;
    
    EnterFunc(bug("NumNewMethods(cl=%s, ifDescr=%p)\n", cl->ClassID, ifDescr));
    
    if (cl->SuperClass)
    {
     	ht = (struct Bucket **)cl->SuperClass->HashTable;
    	for (; ifDescr->MethodTable; ifDescr ++)
    	{
	    ULONG i;
	
    	    for (i = 0; i < ifDescr->NumMethods; i ++)
	    {
	    
    	    	if (!HashLookupStr(ht, (IPTR)ifDescr->MethodTable[i].MethodID))
	    	{
	    	    numnewmethods ++;
	    	}
		
    	   }
	   
       }
       
    }
    else
    {
   	/* This is rootclass, count the methods */
     	ht = (struct Bucket **)cl->HashTable;
    	for (; ifDescr->MethodTable; ifDescr ++)
    	{
	   numnewmethods += ifDescr->NumMethods;
       	}
    }
    ReturnInt ("NumNewMethods", ULONG, numnewmethods);
}

/***************************
**  AllocDispatchTales()  **
***************************/

BOOL AllocDispatchTables(Class *cl, struct InterfaceDescr *ifDescr)
{
    ULONG entries;

    EnterFunc(bug("AllocDispatchTables(cl=%s,ifDescr=%p)\n",
    	cl->ClassID, ifDescr));
    
    entries = NumNewMethods(cl, ifDescr);
    
    cl->HashTable = (struct MethodBucket **)NewHash(entries);
    if (cl->HashTable)
    {
        /* Save hashtable mask for speed */
	cl->HashMask = HashMask(cl->HashTable);
	
	if (cl->SuperClass)
	{
    	    /* Copy the superclass hash table */
	    if ( !CopyHash((struct Bucket **)cl->HashTable
			,(struct Bucket **)cl->SuperClass->HashTable
			,CopyBucket
			,NULL))
	    {
	    	goto failure;
	    }
	}
	
	/* Insert our own methods */
	for (; ifDescr->MethodTable; ifDescr ++)
	{
    	    struct MethodDescr *mtab= ifDescr->MethodTable;
	    ULONG i;
		
	    for (i = 0; i < ifDescr->NumMethods; i ++)
	    {
		struct MethodBucket *b;
		
		/* Method existed in superclass ? */
		b = (struct MethodBucket *)HashLookupStr((struct Bucket **)cl->HashTable
				,(IPTR)mtab[i].MethodID);
		if (b)
		{
		    b->MethodFunc = mtab[i].MethodFunc;
		    b->mClass = cl;
		}
		else
		{
		    /* Must allocate new bucket */
		    struct MethodBucket *new_b;

		    D(bug("Inserting method %s\n", mtab[i].MethodID));
		    
		    new_b = (struct MethodBucket *)malloc( sizeof (struct MethodBucket) );
		    if (!new_b)
		    {
		    	goto failure;
		    }
		    /* Initialize bucket */
		    new_b->MethodID   = mtab[i].MethodID;
		    new_b->MethodFunc = mtab[i].MethodFunc;
		    new_b->mClass     = cl;
		    
		    /* Add bucket to hashtable */
		    InsertBucket((struct Bucket **)cl->HashTable, (struct Bucket *)new_b);
		}

	    } /* for (each method in methodtable) */
		
	} /* for (each interface) */
	
    }
    ReturnBool ("AllocDispatchTables", TRUE);
failure:
    FreeHash((struct Bucket **)cl->HashTable, FreeBucket);
    ReturnBool ("AllocDispatchTables", FALSE);
    
}

VOID FreeDispatchTables(Class *cl)
{
    FreeHash((struct Bucket **)cl->HashTable, FreeBucket);
    
    return;
}


/**************************
**  Hash handling hooks  **
**************************/
#define MB(x) ((struct MethodBucket *)x)
static struct Bucket *CopyBucket(struct Bucket *old_b)
{
    struct MethodBucket *new_b;
    
    new_b = (struct MethodBucket *)malloc(sizeof (struct MethodBucket) );
    if (new_b)
    {
    	new_b->MethodID	  = MB(old_b)->MethodID;
	new_b->MethodFunc = MB(old_b)->MethodFunc;
	new_b->mClass	  = MB(old_b)->mClass;
	return ((struct Bucket *)new_b);
    }
    return (NULL);
}

static VOID FreeBucket(struct Bucket *b)
{
    free(b);
}
