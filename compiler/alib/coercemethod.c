/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: amiga.lib function CoerceMethod()
    Lang: english
*/
#include <intuition/classes.h>
#include <stdarg.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
#include <clib/alib_protos.h>

	IPTR CoerceMethodA (

/*  SYNOPSIS */
	Class  * cl,
	Object * obj,
	Msg	 message)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    HISTORY
	28.11.96    ada created

******************************************************************************/
{
    return CallHookPkt ((struct Hook *)cl, obj, message);
} /* CoerceMethodA */

ULONG CoerceMethod (Class * cl, Object * obj, ULONG MethodID, ...)
{
    AROS_SLOWSTACKMETHODS_PRE(MethodID)
    retval = CallHookPkt ((struct Hook *)cl
	, obj
	, AROS_SLOWSTACKMETHODS_ARG(MethodID)
    );
    AROS_SLOWSTACKMETHODS_POST
} /* CoerceMethod */

