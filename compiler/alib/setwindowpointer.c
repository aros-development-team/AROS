/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain the closes pen to a given color
    Lang: english
*/

#include <exec/types.h>
#define AROS_TAGRETURNTYPE void
#define AROS_TAGRETURNTYPEVOID

#include "alib_intern.h"

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>

	void SetWindowPointer (

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
