/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <stdarg.h>
#include <libraries/reqtools.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	ULONG rtEZRequest (

/*  SYNOPSIS */
	char *bodyfmt,
	char *gadfmt,
	struct rtReqInfo *reqinfo,
	struct TagItem *taglist,
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
#warning FIXME: should use vararg macros!!!!
    return rtEZRequestA(bodyfmt, gadfmt, reqinfo, &taglist + 1, taglist);
    
} /* rtEZRequest */
