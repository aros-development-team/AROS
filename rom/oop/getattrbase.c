/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: OOP function GetAttrBase
    Lang: english
*/

#include "intern.h"
#include "hash.h"
/*****************************************************************************

    NAME */
#include <proto/exec.h>
#include <exec/memory.h>
#include <aros/libcall.h>

#include <aros/debug.h>

	AROS_LH1(ULONG, GetAttrBase,

/*  SYNOPSIS */
	AROS_LHA(STRPTR  	, stringID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 6, OOP)

/*  FUNCTION
	Maps a globally unique string interface ID into
	a numeric AttrBase ID that is unique on
	pr. machine basis.
	

    INPUTS
    	stringID	- globally unique interface identifier.

    RESULT
    	Numeric AttrBase that is unique for this machine.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    /* Look up ID */
    struct iid_bucket *idb;
    struct HashTable *iidtable = GetOBase(OOPBase)->ob_IIDTable;
    ULONG base = -1UL;
    
    EnterFunc(bug("GetAttrBase(stringID=%s)\n", stringID));
    
    
    /* Has ID allready been mapped to a numeric ID ? */
    idb = (struct iid_bucket *)iidtable->Lookup(iidtable, (IPTR)stringID, GetOBase(OOPBase));
    if (idb)
    {

    	/* If so, it has been stored in the hashtable, and we have 
    	** to return the same numeric ID now.
	*/
	if (idb->attrbase == -1UL)
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
	idb = AllocMem(sizeof (struct iid_bucket), MEMF_ANY);
	if (idb)
	{
	    idb->interface_id = AllocVec(strlen(stringID) + 1, MEMF_ANY);
	    if (idb->interface_id)
	    {
	    	D(bug("Allocated bucket\n"));
	    	strcpy(idb->interface_id, stringID);
		
		/* Get next free ID, and increase the free ID to mark it as used */
		base = idb->attrbase = GetOBase(OOPBase)->ob_CurrentAttrBase ++;
		
		base <<= NUM_METHOD_BITS;
		idb->methodbase = -1UL;
		
		/* Insert bucket into hash table */
		InsertBucket(iidtable, (struct Bucket *)idb, GetOBase(OOPBase));
	    }
	    else
	    {
	    	FreeMem(idb, sizeof (struct iid_bucket));
		
/* Throw exception here */		
		base = -1UL;
	    }
	}
    }
    
    ReturnInt ("GetAttrBase", ULONG, base);
    
    AROS_LIBFUNC_EXIT

} /* GetID  */

