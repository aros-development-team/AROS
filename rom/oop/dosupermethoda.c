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

	AROS_LH3(IPTR, DoSuperMethodA,

/*  SYNOPSIS */
	AROS_LHA(struct IClass  *, class, 	A0),
	AROS_LHA(Object         *, object, 	A1),
	AROS_LHA(Msg		 , msg, 	A2),

/*  LOCATION */
	struct Library *, OOPBase, 7, OOP)

/*  FUNCTION
	Pass a method to the superclass.

    INPUTS
    	class	- The class of whose superclass, the method wil
		  dispatch the method.
	object	- The object the method is invoked on.
	msg	- The parameters passed to the method.

    RESULT
    	Dependent on the method implementation.

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
    
    /* this macro is defined in intern.h */
    IntCallMethod(((struct IntClass *)class)->SuperClass, object, msg);
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
