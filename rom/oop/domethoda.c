/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Invoke a method.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH2(IPTR, DoMethodA,

/*  SYNOPSIS */
	AROS_LHA(Object         *, object,	A0),
	AROS_LHA(Msg		 , msg,		A1),

/*  LOCATION */
	struct Library *, OOPBase, 8, OOP)

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
    struct IntClass *cl = (struct IntClass *)OCLASS(object);
    
    IntCallMethod(cl, object, msg);
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
