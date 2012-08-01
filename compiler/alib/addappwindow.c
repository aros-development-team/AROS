/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of workbench.library/AddAppWindowA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE struct AppWindow *
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/workbench.h>

	struct AppWindow * AddAppWindow (

/*  SYNOPSIS */
	IPTR id,
	IPTR  userdata,
	struct Window * window,
	struct MsgPort * msgport,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of workbench.library/AddAppWindowA().
        For information see workbench.library/AddAppWindowA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        workbench.library/AddAppWindowA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = AddAppWindowA (id, userdata, window, msgport, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* AddAppWindow */
