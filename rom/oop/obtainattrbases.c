/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: OOP function ObtainAttrBases
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

	AROS_LH1(BOOL, ObtainAttrBases,

/*  SYNOPSIS */
	AROS_LHA(struct ABDescr *, abd, A0),

/*  LOCATION */
	struct Library *, OOPBase, 18, OOP)

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

    struct ABDescr *d;
    
    for (d = abd; d->interfaceID; d ++)
    {
        *d->attrBase = ObtainAttrBase(d->interfaceID);
	
	if ( *d->attrBase == 0 )
	{
	    /* Clear all other attrbase values */
	    d ++;
	    while (d->interfaceID) {
	    	*d->attrBase = 0;
	    }
	    ReleaseAttrBases(abd);
	    return FALSE;
	}   
    }
    return TRUE;
    
    AROS_LIBFUNC_EXIT

} /* GetID  */

