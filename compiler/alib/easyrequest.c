/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    va_list args;
    LONG    rc;

    va_start (args, idcmpPtr);

    rc = EasyRequestArgs (window, easyStruct, idcmpPtr, (APTR)args);

    va_end (args);

    return rc;
    AROS_LIBFUNC_EXIT
} /* EasyRequest */
