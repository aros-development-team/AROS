/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: OOP function ReleaseAttrBase
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

	AROS_LH1(VOID, ReleaseAttrBase,

/*  SYNOPSIS */
	AROS_LHA(STRPTR  	, interfaceID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 9, OOP)

/*  FUNCTION
	Release an AttrBase previosly obtained with
	ObtainAttrBase()
	

    INPUTS
    	interfaceID	- globally unique interface identifier.
			  for which to release an attrbase.

    RESULT
    	None.

    NOTES
    	The call must be paired wit ObtainAttrBase().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    
    EnterFunc(bug("ReleaseAttrBase(interfaceID=%s)\n", interfaceID));
    
    release_idbucket(interfaceID, GetOBase(OOPBase));
    
    ReturnVoid ("ReleaseAttrBase");
    
    AROS_LIBFUNC_EXIT

} /* GetID  */

