/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a new OOP object
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH3(IPTR, CoerceMethodA,

/*  SYNOPSIS */
	AROS_LHA(Class          *, class,  	A0),
	AROS_LHA(Object         *, object, 	A1),
	AROS_LHA(Msg		 , msg, 	A2),

/*  LOCATION */
	struct Library *, OOPBase, 9, OOP)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    IntCallMethod(((struct IntClass *)class), object, msg);
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
