/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP function OOP_ObtainAttrBase
    Lang: english
*/

#include "intern.h"
#include "hash.h"
/*****************************************************************************

    NAME */
#include <string.h>

#include <proto/exec.h>
#include <exec/memory.h>
#include <aros/libcall.h>

#include <aros/debug.h>

	AROS_LH1(OOP_AttrBase, OOP_ObtainAttrBase,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR  	, interfaceID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 6, OOP)

/*  FUNCTION
	Maps a globally unique string interface ID into
	a numeric AttrBase ID that is unique on a
	per machine basis. The AttrBase can be combined
	with attribute offsets to generate attribute IDs.

    INPUTS
    	interfaceID	- globally unique interface identifier.
			  for which to obtain an attrbase.

    RESULT
    	Numeric AttrBase that is unique for this machine.
	A return value of 0 means that the call failed.

    NOTES
    	Obtained attrbases should be released with ReleaseAttrBase().

    EXAMPLE
	#define aTimer_CurrentTime    (__AB_Timer + aoTime_CurrentTime)
	
	..
	__AB_Timer = OOP_ObtainAttrBase(IID_Timer);
	
	SetAttrs(timer, aTimer_CurrentTime, "10:37:00");
	

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    /* Look up ID */
    struct iid_bucket *idb;
    struct HashTable *iidtable = GetOBase(OOPBase)->ob_IIDTable;
    ULONG base = (ULONG)-1;
    
    EnterFunc(bug("OOP_ObtainAttrBase(interfaceID=%s)\n", interfaceID));
    
    ObtainSemaphore(&GetOBase(OOPBase)->ob_IIDTableLock);
    
    
    /* Has ID already been mapped to a numeric ID? */
    idb = (struct iid_bucket *)iidtable->Lookup(iidtable, (IPTR)interfaceID, GetOBase(OOPBase));
    if (idb)
    {

    	/* If so, it has been stored in the hashtable, and we have 
    	** to return the same numeric ID now.
	*/
	if (idb->attrbase == (ULONG)-1)
	{
	    idb->attrbase = GetOBase(OOPBase)->ob_CurrentAttrBase ++;
	}
	
    	base = idb->attrbase;
	base <<= NUM_METHOD_BITS;

	D(bug("Bucket found: id=%ld\n", base));
    }
    else
    {
    
    	D(bug("No existing bucket\n"));
	
	
    	/* If not, then map it and create a new bucket in the
	** hashtable to store it
	*/
	idb = AllocMem(sizeof (struct iid_bucket), MEMF_ANY|MEMF_CLEAR);
	if (idb)
	{
	    idb->interface_id = AllocVec(strlen(interfaceID) + 1, MEMF_ANY);
	    if (idb->interface_id)
	    {
	    	D(bug("Allocated bucket\n"));
	    	strcpy(idb->interface_id, interfaceID);
		
		/* Get next free ID, and increase the free ID count to mark it as used */
		base = idb->attrbase = ++ GetOBase(OOPBase)->ob_CurrentAttrBase;
		
		base <<= NUM_METHOD_BITS;
		
		/* Methodbase not inited yet */
		idb->methodbase = (ULONG)-1;
		
		/* Insert bucket into hash table */
		InsertBucket(iidtable, (struct Bucket *)idb, GetOBase(OOPBase));
	    }
	    else
	    {
	    	FreeMem(idb, sizeof (struct iid_bucket));
		
/* Throw exception here ? */		
		base = 0UL;
	    }
	}
    }
    
    if (base)
    {
    	/* Increase refcount of bucket */
	idb->refcount ++;
    }

    ReleaseSemaphore(&GetOBase(OOPBase)->ob_IIDTableLock);
    
    ReturnInt ("OOP_ObtainAttrBase", AttrBase, base);
    
    AROS_LIBFUNC_EXIT

} /* OOP_ObtainAttrBase  */

