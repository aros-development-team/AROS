/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of workbench.library/AddAppIconA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE struct AppIcon *
#include <dos/bptr.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/workbench.h>

	struct AppIcon * AddAppIcon (

/*  SYNOPSIS */
	ULONG id,
	IPTR  userdata,
	CONST_STRPTR text,
	struct MsgPort * msgport,
	BPTR lock,
	struct DiskObject * diskobj,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of workbench.library/AddAppIconA().
        For information see workbench.library/AddAppIconA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        workbench.library/AddAppIconA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = AddAppIconA (id, userdata, text, msgport, lock, diskobj, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* AddAppIcon */
