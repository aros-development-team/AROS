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
	Invokes a method on supplied object as if it
	was an instance of supplied class. Ie. the supplied
	class will be the first one to dispatch the method.

    INPUTS
    	class	- Pointer to class that should dispatch the method.
	object	- The object that the method is invoked on.
	msg	- The parameters sent to the method.

    RESULT
    	Dependant on the method.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	DoSuperMethodA()

    INTERNALS
    	See intern.h - IntCallMethod macro.

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    /* Macro below defined in intern.h */
    IntCallMethod(((struct IntClass *)class), object, msg);
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
