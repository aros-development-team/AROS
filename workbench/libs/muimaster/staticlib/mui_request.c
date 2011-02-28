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
	const char *title,
	const char *gadgets,
	const char *format,
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
    return MUI_RequestA(app, win, flags, title, gadgets, format, (&format)+1);
} /* MUI_Request */
