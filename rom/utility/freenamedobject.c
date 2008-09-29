/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FreeNamedObject() - Free a NamedObject.
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
	#include <proto/utility.h>

	AROS_LH1(void, FreeNamedObject,

/*  SYNOPSIS */
	AROS_LHA(struct NamedObject *, object, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 41, Utility)

/*  FUNCTION
	Frees a NamedObject previously allocated by AllocNamedObject().

    INPUTS
	object	    -	The NamedObject that you wish to free.

    RESULT
	The memory used by the NamedObject will be returned to the
	systems free memory pool.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	utility/name.h, AllocNamedObjectA()

    INTERNALS
	AllocMem:
	    NameSpace;
	AllocVec:
	    IntNamedObject, Object.

	FreeNamedObject() may have to deal with only a partially allocated
	object from AllocNamedObject() which has come across an error.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Adapted for AROS code.
	08-10-96    iaint   Modified after discussion in AROS-DEV.
	19-10-96    iaint   Finished above.
	16-04-01    iaint   Combined the memory for the name and
			    IntNamedObject as an optimisation.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if( object )
    {
	struct IntNamedObject *no = GetIntNamedObject(object);

	if(object->no_Object && no->no_FreeObject)
	    FreeVec(object->no_Object);

	if(no->no_NameSpace)
	    FreeMem(no->no_NameSpace, sizeof(struct NameSpace));

	FreeVec(object);
    }
    AROS_LIBFUNC_EXIT

} /* FreeNamedObject */
