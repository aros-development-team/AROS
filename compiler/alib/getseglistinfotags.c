/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of dos.library/GetSegListInfoTags()
    Lang: english
*/

#define AROS_TAGRETURNTYPE LONG
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/dos.h>

	LONG GetSegListInfoTags (

/*  SYNOPSIS */
	BPTR seglist,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of dos.library/GetSegListInfo().
        For information see dos.library/GetSegListInfo().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/GetSegListInfo()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = GetSegListInfo (seglist, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* GetSegListInfoTags */
