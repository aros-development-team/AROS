/*
    (C) 1995-96 AROS - The Amiga Research OS
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

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    va_list args;
    struct Window * rc;

    va_start (args, IDCMP);

    rc = BuildEasyRequestArgs (RefWindow, easyStruct, IDCMP, (APTR)args);

    va_end (args);

    return rc;
    AROS_LIBFUNC_EXIT
} /* BuildEasyRequest */
