/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BOOPSI functions CoerceMethodA() and CoerceMethod()
    Lang: english
*/

#include <intuition/classes.h>
#include <stdarg.h>
#include "alib_intern.h"
#include <proto/alib.h>

/******************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

	IPTR CoerceMethodA (

/*  SYNOPSIS */
	Class  * cl,
	Object * obj,
	Msg	 message)

/*  FUNCTION
	Invokes a method on a BOOPSI object, as if this was a object, inherited
	from the class passed in. Ie the dispatcher of the this class is called
        instead of the objects classes dispatcher.

    INPUTS
	cl - Class, which dispatcher is to be called.
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

    HISTORY
	28.11.96    ada created

******************************************************************************/
{
    if ((!obj) || (!cl))
        return 0L;
    return CallHookPkt ((struct Hook *)cl, obj, message);
} /* CoerceMethodA */

ULONG CoerceMethod (Class * cl, Object * obj, ULONG MethodID, ...)
{
    AROS_SLOWSTACKMETHODS_PRE(MethodID)
    if ((!obj) || (!cl))
        retval = 0L;
    else
        retval = CallHookPkt ((struct Hook *)cl
	    , obj
	    , AROS_SLOWSTACKMETHODS_ARG(MethodID)
        );
    AROS_SLOWSTACKMETHODS_POST
} /* CoerceMethod */

