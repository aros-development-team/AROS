/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of BuildEasyRequestArgs() (intuition.library)
    Lang: english
*/
#include <stdarg.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>
#include <intuition/intuition.h>

	struct Window * BuildEasyRequest (

/*  SYNOPSIS */
	struct Window	  * RefWindow,
	struct EasyStruct * easyStruct,
	ULONG		    IDCMP,
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	intuition.library/BuildEasyRequestArgs()
    
    INTERNALS

    HISTORY

*****************************************************************************/
{
    va_list args;
    struct Window * rc;

    va_start (args, IDCMP);

    rc = BuildEasyRequestArgs (RefWindow, easyStruct, IDCMP, (APTR)args);

    va_end (args);

    return rc;
} /* BuildEasyRequest */
