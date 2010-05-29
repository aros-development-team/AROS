/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of datatypes.library/SetDTAttrsA()
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

	ULONG SetDTAttrs (

/*  SYNOPSIS */
	Object * o,
	struct Window * win,
	struct Requester *req,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of datatypes.library/SetDTAttrsA().
        For information see datatypes.library/SetDTAttrsA().

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
    SetDTAttrsA (o, win, req, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SetDTAttrs */
