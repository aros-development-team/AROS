/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Obtain the closes pen to a given colot
    Lang: english
*/
#include <exec/types.h>
#define AROS_TAGRETURNTYPE LONG

#include "alib_intern.h"

extern struct GfxBase * GfxBase;

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <graphics/view.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/graphics.h>

	LONG ObtainBestPen (

/*  SYNOPSIS */
	struct ColorMap * cm,
	LONG              R,
	LONG              G,
	LONG              B,
	ULONG		  tag1,
	...		  )

/*  FUNCTION
        Varargs version of graphics.library/ObtainBestPenA().

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
    retval = ObtainBestPenA (cm, R,G,B, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* ObtainBestPen */
