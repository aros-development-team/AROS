/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
	Invokes a method on a BOOPSI object, as if the object were an instance
	of the superclass of the class passed in. That is, the dispatcher of
	the superclass is called instead of the object's class's dispatcher
	(assuming the specified class is the object's own class).

    INPUTS
	cl - Class whose superclass's dispatcher is to be called. This is
	    typically the object's own class.
	obj - The object on which the method is to be performed.
	message - The message. The first field is the same for all methods and
		  specifies which method is to be invoked (see
		  <intuition/classusr.h>).

    RESULT
	Class and method dependent. See the class documentation. A value of 0
	can mean a valid return code but can also mean that a method was not
	supported.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	intuition.library/NewObjectA(), intuition.library/SetAttrsA(),
	intuition.library/GetAttr(), intuition.library/DisposeObject(),
	DoMethodA(), CoerceMethodA(), <intuition/classes.h>

******************************************************************************/
{
    if ((!obj) || (!cl))
        return 0L;
    return CALLHOOKPKT((struct Hook *)cl->cl_Super, obj, message);
} /* DoSuperMethodA() */


IPTR DoSuperMethod (Class * cl, Object * obj, STACKULONG MethodID, ...)
{
    if ((!obj) || (!cl))
        return 0L;
    AROS_SLOWSTACKMETHODS_PRE(MethodID)    
    retval = CALLHOOKPKT((struct Hook *)cl->cl_Super, obj, AROS_SLOWSTACKMETHODS_ARG(MethodID));
    AROS_SLOWSTACKMETHODS_POST
} /* DoSuperMethod()*/
