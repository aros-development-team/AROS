/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdarg.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/muimaster.h>
extern struct Library * MUIMasterBase;

	LONG MUI_Request (

/*  SYNOPSIS */
	APTR app,
	APTR win,
	LONG flags,
	char *title,
	char *gadgets,
	char *format,
	APTR param1,
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
    AROS_SLOWSTACKTAGS_PRE(param1)

    retval = MUI_RequestA(app, win, flags, title, gadgets, format, AROS_SLOWSTACKTAGS_ARG(param1));

    AROS_SLOWSTACKTAGS_POST
    
} /* MUI_Request */
