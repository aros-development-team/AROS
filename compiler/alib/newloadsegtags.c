/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of dos.library/NewLoadSeg()
    Lang: english
*/

#define AROS_TAGRETURNTYPE BPTR
#include <dos/bptr.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/dos.h>

	BPTR NewLoadSegTags (

/*  SYNOPSIS */
	STRPTR file,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of dos.library/NewLoadSeg().
        For information see dos.library/NewLoadSeg().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/NewLoadSeg()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = NewLoadSeg (file, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* NewLoadSegTags */
