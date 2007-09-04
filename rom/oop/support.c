/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <string.h>

#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/lists.h>

#include <proto/oop.h>
#include <oop/oop.h>

#include "intern.h"
#include "hash.h"
#include "private.h"


#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


#define UB(x) ((UBYTE *)x)

#define ClassID PPart.ClassNode.ln_Name




/***************
**  GetIDs()  **
***************/

/* Used to do several string interface ID to numeric interface ID mappings in one call */
BOOL GetIDs(struct IDDescr *idDescr, struct IntOOPBase *OOPBase)
{
    while (idDescr->ID)
    {
    	*(idDescr->Storage) = OOP_ObtainAttrBase(idDescr->ID);
	if (0UL == *(idDescr->Storage))
	    return (FALSE);
	    
	idDescr ++;
    }
    return (TRUE);
}

/*********************
**  hasinterface()  **
*********************/

BOOL hasinterface(OOP_Class *cl, STRPTR interface_id)
{
    ULONG num_methods = 0UL;
    return  (meta_getifinfo((OOP_Object *)cl, interface_id, &num_methods) != NULL) ? TRUE : FALSE;
}

/**********************
**  findinterface()  **
**********************/
struct IFMethod *findinterface(OOP_Class *cl, STRPTR interface_id)
{
    ULONG num_methods = 0UL;
    
    return meta_getifinfo((OOP_Object *)cl, interface_id, &num_methods);
}

/***************************
**  init_mi_methodbase()  **
***************************/
/* Intializes a methodbase to be used for the classes supporting multiple
   interfaces (MI), ie. objects of th IFMeta class.
*/
BOOL init_mi_methodbase(STRPTR interface_id, ULONG *methodbase_ptr, struct IntOOPBase *OOPBase)
{
   ULONG mbase;
   BOOL success;
   EnterFunc(bug("init_mi_methodbase(interface_id=%s)\n", interface_id));
   
   mbase = OOPBase->ob_CurrentMethodBase ++;
   mbase <<= NUM_METHOD_BITS;
   success = init_methodbase(interface_id, mbase, methodbase_ptr, OOPBase);
   if (success)
   {
       if (*methodbase_ptr != mbase)
       {
           /* Methodbase had allready been inited, reset current ID */
	   OOPBase->ob_CurrentMethodBase --;
       }
   }
   ReturnBool ("init_mi_methodbase", success);
   
}

/************************
**  init_methodbase()  **
************************/

/* Adds the local ID equivalent to the global string ID 
   to the ID hashtable
*/

BOOL init_methodbase(STRPTR interface_id, ULONG methodbase, ULONG *methodbase_ptr, struct IntOOPBase *OOPBase)
{
    BOOL inited = FALSE;
    struct iid_bucket *idb = NULL;
    struct HashTable *iidtable = OOPBase->ob_IIDTable;
    
    
    EnterFunc(bug("init_methodbase(interface_id=%s, methodbase=%ld)\n",
    	interface_id, methodbase));
	
    ObtainSemaphore(&OOPBase->ob_IIDTableLock);
	
    /* Has ID allready been mapped to a methodbase ? */
    idb = (struct iid_bucket *)iidtable->Lookup(iidtable, (IPTR)interface_id, (struct IntOOPBase *)OOPBase);
    if (idb)
    {
	if (idb->methodbase == (ULONG)-1)
	{
	    idb->methodbase = methodbase;
	}
	
	*methodbase_ptr = idb->methodbase;
        inited = TRUE;
	
    }
    else
    {
    
    	D(bug("Initing methodbase...\n"));
	
	
    	/* If not, then map it and create a new bucket in the
	** hashtable to store it
	*/
	idb = AllocMem(sizeof (struct iid_bucket), MEMF_ANY);
	if (idb)
	{
	    idb->interface_id = AllocVec(strlen(interface_id) + 1, MEMF_ANY);
	    if (idb->interface_id)
	    {
	    	D(bug("Allocated bucket\n"));
	    	strcpy(idb->interface_id, interface_id);
		
		/* Get next free ID, and increase the free ID to mark it as used */
		
		idb->methodbase = methodbase;
		*methodbase_ptr = methodbase;
		
		/* Leave attrbase field unitialized */
		idb->attrbase = -1UL;
		
		/* Insert bucket into hash table */
		InsertBucket(iidtable, (struct Bucket *)idb, (struct IntOOPBase *)OOPBase);
		inited = TRUE;
	    }
	    else
	    {
	    	FreeMem(idb, sizeof (struct iid_bucket));
	    }
	}
    }
    if (idb)
    {
    	idb->refcount ++;
    }

    ReleaseSemaphore(&OOPBase->ob_IIDTableLock);
    
    ReturnBool ("init_methodbase", inited);

}

/* Release a interface bucket */
VOID release_idbucket(STRPTR interface_id, struct IntOOPBase *OOPBase)
{
    /* Look up ID */
    struct iid_bucket *idb;
    struct HashTable *iidtable = GetOBase(OOPBase)->ob_IIDTable;

    ObtainSemaphore(&OOPBase->ob_IIDTableLock);
    /* Has ID allready been mapped to a numeric ID ? */
    idb = (struct iid_bucket *)iidtable->Lookup(iidtable, (IPTR)interface_id, OOPBase);
    if (idb)
    {
    	/* Reduce interface bucket's refcount */
	idb->refcount --;
	
	/* Last ref released ? */
	if (idb->refcount == 0)
	{
	    /* Remove and free the bucket */
	    RemoveBucket(iidtable, (struct Bucket *)idb);
	    FreeVec(idb->interface_id);
	    FreeMem(idb, sizeof (struct iid_bucket));
	}
    }
    
    ReleaseSemaphore(&OOPBase->ob_IIDTableLock);
    
    ReturnVoid ("ReleaseAttrBase");
}

/* Increase an idbucket's refcount, to lock it. 
   Calling this function MUST ONLY be used for IFs
   that are known to exist in the IID table
*/
VOID obtain_idbucket(STRPTR interface_id, struct IntOOPBase *OOPBase)
{
    struct iid_bucket *idb;
    struct HashTable *iidtable = GetOBase(OOPBase)->ob_IIDTable;

    ObtainSemaphore(&OOPBase->ob_IIDTableLock);
    /* Has ID allready been mapped to a numeric ID ? */
    idb = (struct iid_bucket *)iidtable->Lookup(iidtable, (IPTR)interface_id, OOPBase);
    /* Reduce interface bucket's refcount */
    idb->refcount ++;
    ReleaseSemaphore(&OOPBase->ob_IIDTableLock);
    
    return;
}
