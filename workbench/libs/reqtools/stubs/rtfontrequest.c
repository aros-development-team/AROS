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
	ULONG rtFontRequest (

/*  SYNOPSIS */
	struct rtFontRequester *fontreq,
	const char *title,
	Tag tag1, 
	...)

/*  FUNCTION
        Request a font and optional font style.

    INPUTS
        fontreq - Font requester allocated with rtAllocRequest().
        title   - Requester window title.
        tag1    - First requester tag; terminate with TAG_DONE.

    RESULT
        Nonzero when a font is selected; zero on cancellation.

    NOTES
        The selected font attributes are stored in the requester.
        This requester may use DOS and must be called from a process.

    SEE ALSO
        rtFontRequestA(), rtAllocRequest()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtFontRequestA(fontreq, title, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtFontRequest */
