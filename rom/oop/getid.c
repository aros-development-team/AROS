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

    INPUTS

    RESULT

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
    struct IDBucket *idb;
    struct HashTable *idtable = GetOBase(OOPBase)->ob_IDTable;
    ULONG id = 0UL;
    
    EnterFunc(bug("GetID(stringID=%s)\n", stringID));
    
    idb = (struct IDBucket *)idtable->Lookup(idtable, (IPTR)stringID, (struct IntOOPBase *)OOPBase);
    if (idb)
    {
    	id = idb->NumericID;
	D(bug("Bucket found: id=%ld\n", id));
    }
    else
    {
    	D(bug("No existing bucket\n"));
    	/* Must create a new bucket */
	idb = AllocMem(sizeof (struct IDBucket), MEMF_ANY);
	if (idb)
	{
	    idb->StrID = AllocVec(strlen(stringID) + 1, MEMF_ANY);
	    if (idb->StrID)
	    {
	    	D(bug("Allocated bucket\n"));
	    	strcpy(idb->StrID, stringID);
		
		/* Set ID and increase */
		id = idb->NumericID = ++ GetOBase(OOPBase)->ob_CurrentID;
		
		
		InsertBucket(idtable, (struct Bucket *)idb, (struct IntOOPBase *)OOPBase);
	    }
	    else
	    {
	    	FreeMem(idb, sizeof (struct IDBucket));
	    }
	}
    }
    
    ReturnInt ("GetID", ULONG, id << NUM_METHOD_BITS);
    
    AROS_LIBFUNC_EXIT

} /* GetID  */

