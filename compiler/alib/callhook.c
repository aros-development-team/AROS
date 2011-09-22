/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: amiga.lib function CallHook() which doesn't use utility.library
    Lang: english
*/

#define ALIB_NO_INLINE_STDARG

#include <aros/system.h>
#include <stdarg.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
#include <proto/alib.h>

	IPTR CallHookA (

/*  SYNOPSIS */
	struct Hook * hook,
	APTR	      object,
	APTR	      param)

/*  FUNCTION
	Calls a hook with the specified object and parameters.

    INPUTS
	hook - Call this hook.
	object - This is the object which is passed to the hook. The valid
	    values for this parameter depends on the definition of the called
	    hook.
	param - Pass these parameters to the specified object

    RESULT
	The return value depends on the definition of the hook.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CallHook()

******************************************************************************/
{
    return CALLHOOKPKT(hook, object, param);
} /* CallHookA() */

/******************************************************************************

    NAME */
	IPTR CallHook (

/*  SYNOPSIS */
	struct Hook * hook,
	APTR object,
	...)

/*  FUNCTION
	Variadic form of CallHookA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CallHookA()

******************************************************************************/

{
    AROS_SLOWSTACKHOOKS_PRE(object)
    retval = CALLHOOKPKT(hook, object, AROS_SLOWSTACKHOOKS_ARG(object));
    AROS_SLOWSTACKHOOKS_POST
} /* CallHook() */
