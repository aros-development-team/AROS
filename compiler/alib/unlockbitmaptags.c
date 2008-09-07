/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "alib_intern.h"

extern struct Library * CyberGfxBase;

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/cybergraphics.h>

	void UnLockBitMapTags (

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
	cgfx.library/UnLockBitMapTagList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_NR_SLOWSTACKTAGS_PRE(tag1)
    UnLockBitMapTagList(handle, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_NR_SLOWSTACKTAGS_POST
} /* UnLockBitMapTags */
