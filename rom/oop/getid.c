/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: OOP function GetID
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

	AROS_LH1(ULONG, GetID,

/*  SYNOPSIS */
	AROS_LHA(STRPTR  	, stringID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 6, OOP)

/*  FUNCTION
	Maps a globally unique string interface ID into
	a numeric interface ID that is unique on
	pr. machine basis.
	The interface ID is also used as a part of the method ID.
	Programmers must use this to
	initialize global vars that are used for
	as part of methodIDs.

    INPUTS
    	stringID	- globally unique interface identifier.

    RESULT
    	Numeric interface identifier that is unique for this machine.

    NOTES
    	Can also be used for initializing attribute bases.

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
    struct IDBucket *idb;
    struct HashTable *idtable = GetOBase(OOPBase)->ob_IDTable;
    ULONG id = 0UL;
    
    EnterFunc(bug("GetID(stringID=%s)\n", stringID));
    
    
    /* Has ID allready been mapped to a numeric ID ? */
    idb = (struct IDBucket *)idtable->Lookup(idtable, (IPTR)stringID, (struct IntOOPBase *)OOPBase);
    if (idb)
    {

    	/* If so, it has been stored in the hashtable, and we have 
    	** to return the same numeric ID now.
	*/
    	id = idb->NumericID;
	D(bug("Bucket found: id=%ld\n", id));
    }
    else
    {
    
    	D(bug("No existing bucket\n"));
	
	
    	/* If not, then map it and create a new bucket in the
	** hashtable to store it
	*/
	idb = AllocMem(sizeof (struct IDBucket), MEMF_ANY);
	if (idb)
	{
	    idb->StrID = AllocVec(strlen(stringID) + 1, MEMF_ANY);
	    if (idb->StrID)
	    {
	    	D(bug("Allocated bucket\n"));
	    	strcpy(idb->StrID, stringID);
		
		/* Get next free ID, and increase the free ID to mark it as used */
		id = idb->NumericID = ++ GetOBase(OOPBase)->ob_CurrentID;
		
		/* Insert bucket into hash table */
		InsertBucket(idtable, (struct Bucket *)idb, (struct IntOOPBase *)OOPBase);
	    }
	    else
	    {
	    	FreeMem(idb, sizeof (struct IDBucket));
	    }
	}
    }
    
    /* The ID must be left-shifted to make place for method offsets */
    ReturnInt ("GetID", ULONG, id << NUM_METHOD_BITS);
    
    AROS_LIBFUNC_EXIT

} /* GetID  */

