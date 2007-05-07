/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: OOP function OOP_ReleaseAttrBase
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

	AROS_LH1(VOID, OOP_ReleaseAttrBase,

/*  SYNOPSIS */
	AROS_LHA(STRPTR  	, interfaceID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 9, OOP)

/*  FUNCTION
	Release an OOP_AttrBase previosly obtained with
	OOP_ObtainAttrBase()
	

    INPUTS
    	interfaceID	- globally unique interface identifier.
			  for which to release an attrbase.

    RESULT
    	None.

    NOTES
    	The call must be paired wit OOP_ObtainAttrBase().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    
    EnterFunc(bug("OOP_ReleaseAttrBase(interfaceID=%s)\n", interfaceID));
    
    release_idbucket(interfaceID, GetOBase(OOPBase));
    
    ReturnVoid ("OOP_ReleaseAttrBase");
    
    AROS_LIBFUNC_EXIT

} /* OOP_ReleaseAttrBase  */

