/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: OOP function GetMethodID
    Lang: english
*/
#include "intern.h"
/*****************************************************************************

    NAME */
#include <proto/exec.h>
#include <proto/oop.h>
#include <exec/memory.h>
#include <aros/libcall.h>

#include <aros/debug.h>
#include "hash.h"

	AROS_LH2(MethodID, GetMethodID,

/*  SYNOPSIS */
	AROS_LHA(STRPTR  	, interfaceID, A0),
	AROS_LHA(ULONG  	, methodOffset, D0),

/*  LOCATION */
	struct Library *, OOPBase, 7, OOP)

/*  FUNCTION
	Maps a globally unique full method ID
	(Interface ID + method offset) into
	a numeric method ID.

    INPUTS
    	interfaceID	- globally unique interface identifier.
	methodOffset	- offset to the method in this interface.
	

    RESULT
    	Numeric method identifier that is unique for this machine.

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
    ULONG mid = 0UL;
    struct iid_bucket *idb;
    struct HashTable *iidtable = GetOBase(OOPBase)->ob_IIDTable;
    
    EnterFunc(bug("GetMethodID(interfaceID=%s, methodOffset=%ld)\n",
    	interfaceID, methodOffset));
    
/* #warning doesn't handle failures. (Should throw exception)
*/
    idb = (struct iid_bucket *)iidtable->Lookup(iidtable, (IPTR)interfaceID, GetOBase(OOPBase));
    if (idb)
    {
        /* Should throw eception here if metodbase == -1UL */
        mid = idb->methodbase + methodOffset;

    	ReturnInt ("GetMethodID", ULONG, mid);
    }
    
    /* Should throw exception here */
    
    /* The ID must be left-shifted to make place for method offsets */
    ReturnInt ("GetMethodID", ULONG, -1UL);
    
    AROS_LIBFUNC_EXIT

} /* GetMethodID  */

