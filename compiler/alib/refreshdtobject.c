/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of datatypes.library/RefreshDTObjectA()
    Lang: english
*/

#define AROS_TAGRETURNTYPEVOID
#include <intuition/classusr.h>
#include <utility/tagitem.h>

extern struct Library *DataTypesBase;

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/datatypes.h>

	void RefreshDTObject (

/*  SYNOPSIS */
	Object * o,
	struct Window * win,
	struct Requester *req,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of datatypes.library/RefreshDTObjectA().
        For information see datatypes.library/RefreshDTObjectA().

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
    RefreshDTObjectA (o, win, req, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* RefreshDTObject */
