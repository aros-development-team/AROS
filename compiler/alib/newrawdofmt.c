/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: vnewrawdofmt.c 26100 2007-05-15 20:06:02Z verhaegs $

    Desc: Format a string and emit it.
    Lang: english
*/
#include <exec/execbase.h>
#include <stdarg.h>
#define NO_INLINE_STDARG
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	STRPTR NewRawDoFmt(

/*  SYNOPSIS */
	CONST_STRPTR FormatString,
	VOID_FUNC PutChProc,
	APTR PutChData,
	... )

/*  FUNCTION
        This is the varargs version of exec.library/VNewRawDoFmt().
        For information see exec.library/VNewRawDoFmt().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    STRPTR retval;
    va_list args;
    
    va_start(args, PutChData);
    retval = VNewRawDoFmt(FormatString, PutChProc, PutChData, args);
    va_end(args);
    return retval;

} /* NewRawDoFmt */
