/*
    Copyright (C) 1997-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Obtain the closes pen to a given color
    Lang: english
*/
#include <exec/types.h>
#define AROS_TAGRETURNTYPE BOOL

#include "alib_intern.h"

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>

	LONG SetWindowPointer (

/*  SYNOPSIS */
        struct Window   * window,
	ULONG		  tag1,
	...		  )

/*  FUNCTION
        Varargs version of intuition.library/SetWindowPointerA().

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
    AROS_SLOWSTACKTAGS_PRE(tag1)
    SetWindowPointerA (window, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SetWindowPointer */
