/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of amigaguide.library/SendAmigaGuideCmdA()
    Lang: english
*/

#define AROS_TAGRETURNTYPE BOOL
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/amigaguide.h>

	BOOL SendAmigaGuideCmd (

/*  SYNOPSIS */
	AMIGAGUIDECONTEXT handle,
	STRPTR cmd,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of amigaguide.library/SendAmigaGuideCmdA().
        For information see amigaguide.library/SendAmigaGuideCmdA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        amigaguide.library/SendAmigaGuideCmdA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SendAmigaGuideCmdA (handle, cmd, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SendAmigaGuideCmd */
