/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_TAGRETURNTYPEVOID
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

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    UnLockBitMapTagList(handle, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* UnLockBitMapTags */
