/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>

	LONG EasyRequest (

/*  SYNOPSIS */
	struct Window	  * window,
	struct EasyStruct * easyStruct,
	ULONG		  * idcmpPtr,
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    va_list args;
    LONG    rc;

    va_start (args, idcmpPtr);

    rc = EasyRequestArgs (window, easyStruct, idcmpPtr, (APTR)args);

    va_end (args);

    return rc;
} /* EasyRequest */
