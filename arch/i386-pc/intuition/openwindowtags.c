/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Open a window
    Lang: english
*/
#define AROS_TAGRETURNTYPE  struct Window *

#include <exec/execbase.h>
#include <proto/exec.h>

#include <intuition/intuitionbase.h>
#include "alib_intern.h"

#ifdef SysBase
#undef SysBase
#endif
#define SysBase (*(struct ExecBase**)4L)

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>

	struct Window * OpenWindowTags (

/*  SYNOPSIS */
	struct NewWindow * newWindow,
	ULONG		   tag1,
	...		   )

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    struct IntuitionBase *IntuitionBase=
	(struct IntuitionBase*)FindName(&SysBase->LibList,"intuition.library");
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = OpenWindowTagList (newWindow, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* OpenWindowTags */
