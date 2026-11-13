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
	ULONG rtScreenModeRequest (

/*  SYNOPSIS */
	struct rtScreenModeRequester *screenmodereq,
	char *title,
	Tag tag1, 
	...)

/*  FUNCTION
        Request a screen mode and its display parameters.

    INPUTS
        screenmodereq - Screen-mode requester allocated with
                        rtAllocRequest().
        title         - Requester window title.
        tag1          - First requester tag; terminate with TAG_DONE.

    RESULT
        Nonzero when the user accepts the selection; zero on cancellation.

    NOTES
        The chosen display mode, dimensions, and depth are stored in
        the requester structure.

    SEE ALSO
        rtScreenModeRequestA(), rtAllocRequest()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtScreenModeRequestA(screenmodereq, title, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtScreenModeRequest */
