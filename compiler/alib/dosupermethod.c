/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define ALIB_NO_INLINE_STDARG

#include <intuition/classes.h>
#include <stdarg.h>
#include <proto/alib.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/alib.h>

	IPTR DoSuperMethodA (

/*  SYNOPSIS */
	Class  * cl,
	Object * obj,
	Msg	 message)

/*  FUNCTION
	Invokes a method on a BOOPSI object, as if this was a object, inherited
	from the superclass of the class passed in. Ie the dispatcher of the
        superclass is called instead of the objects classes dispatcher.

    INPUTS
	cl - Class, which superclasses dispatcher is to be called.
	obj - The object, on which the method is to be performed on.
	message - The message. The first field is the same for all methods and
		  specifies which method is to be invokes (see
		  <intuition/classusr.h>).

    RESULT
	Class and method depending. See the class documentation. A value of 0
	can mean a valid return code but can also mean that a method was not
	supported.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	boopsi.library/NewObjectA(), boopsi.library/SetAttrsA(),
	boopsi.library/GetAttr(), boopsi.library/DisposeObject(), DoMethodA(),
        CoerceMethodA(), <intuition/classes.h>

******************************************************************************/
{
    if ((!obj) || (!cl))
        return 0L;
    return CALLHOOKPKT((struct Hook *)cl->cl_Super, obj, message);
} /* DoSuperMethodA() */


#ifndef NO_LINEAR_VARARGS

IPTR DoSuperMethod (Class * cl, Object * obj, STACKULONG MethodID, ...)
{
    if ((!obj) || (!cl))
        return 0L;
    AROS_SLOWSTACKMETHODS_PRE(MethodID)    
    retval = CALLHOOKPKT((struct Hook *)cl->cl_Super, obj, AROS_SLOWSTACKMETHODS_ARG(MethodID));
    AROS_SLOWSTACKMETHODS_POST
} /* DoSuperMethod()*/

#endif
