/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_TAGRETURNTYPE APTR
#include "alib_intern.h"

extern struct Library * CyberGfxBase;

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/cybergraphics.h>

	APTR LockBitMapTags (

/*  SYNOPSIS */
	APTR handle,
	Tag tag1,
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	cgfx.library/LockBitMapTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = LockBitMapTagList(handle, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* LockBitMapTags */
