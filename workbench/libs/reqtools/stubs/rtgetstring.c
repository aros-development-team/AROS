/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

#define AROS_TAGRETURNTYPE ULONG
#include <utility/tagitem.h>
#include <libraries/reqtools.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	ULONG rtGetString (

/*  SYNOPSIS */
	UBYTE *buffer,
	ULONG maxchars,
	const char *title,
	struct rtReqInfo *reqinfo,
	Tag tag1, 
	...)

/*  FUNCTION
        Request a line of text.

    INPUTS
        buffer   - Input text and destination for the edited string.
        maxchars - Maximum character count, excluding the terminator.
        title    - Requester window title.
        reqinfo  - Optional requester information.
        tag1     - First requester tag; terminate with TAG_DONE.

    RESULT
        Response code from rtGetStringA().

    NOTES
        The destination must accommodate maxchars characters and the
        terminating NUL. Empty-input behavior depends on RTGS_AllowEmpty;
        see rtGetStringA().

    SEE ALSO
        rtGetStringA()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtGetStringA(buffer, maxchars, title, reqinfo, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtGetString */
