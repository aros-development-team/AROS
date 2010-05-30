/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of datatypes.library/GetDTAttrsA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE ULONG
#include <intuition/classusr.h>
#include <utility/tagitem.h>

extern struct Library *DataTypesBase;

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/datatypes.h>

	ULONG GetDTAttrs (

/*  SYNOPSIS */
	Object * o,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of datatypes.library/GetDTAttrsA().
        For information see datatypes.library/GetDTAttrsA().

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
    retval = GetDTAttrsA (o, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* GetDTAttrs */
