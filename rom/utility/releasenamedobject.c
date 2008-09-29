/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReleaseNamedObject()
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
	#include <proto/utility.h>

	AROS_LH1(void, ReleaseNamedObject,

/*  SYNOPSIS */
	AROS_LHA(struct NamedObject *, object, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 43, Utility)

/*  FUNCTION
	Releases a NamedObject that you previously obtained by calling
	FindNamedObject.

    INPUTS
	object	    -	The NamedObject to release.

    RESULT
	The NamedObject will be released from your possession, and if it
	is ready to be deallocated, then the NamedObject will be freed.

    NOTES
	WARNING: You really should actually have found the NamedObject
	    first (that is with FindNamedObject()) before calling this
	    function. Failure to take heed of this will cause memory
	    use problems.

    EXAMPLE
	struct NamedObject *nObj, *myNameSpace;

	if( nObj = FindNamedObject( myNameSpace, "Some Name", NULL ) )
	{
	    \*
		Here you do whatever you want. However The NamedObject
		structure should generally be treated READ-ONLY
	    *\

	    ReleaseNamedObject( nObj );
	}

    BUGS

    SEE ALSO
	utility/name.h, FindNamedObject()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Adapted for AROS 1.5+.
	18-10-96    iaint   Changed for different format NamedObjects.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntNamedObject *no;

    if( object )
    {
	Forbid();
	no = GetIntNamedObject( object );

	/* if(no more users) */
	if( --no->no_UseCount == 0 )
	{
	    if( no->no_FreeMessage )    RemNamedObject( object, NULL );
	}
	Permit();
    }

    AROS_LIBFUNC_EXIT

} /* ReleaseNamedObject */
