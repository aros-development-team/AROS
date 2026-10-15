/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

#define AROS_TAGRETURNTYPE LONG
#include <utility/tagitem.h>
#include <libraries/reqtools.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	LONG rtPaletteRequest (

/*  SYNOPSIS */
	const char *title,
	struct rtReqInfo *reqinfo,
	Tag tag1, 
	...)

/*  FUNCTION
        Display a requester for editing or choosing a screen color.

    INPUTS
        title   - Requester window title.
        reqinfo - Optional requester information.
        tag1    - First requester tag; terminate with TAG_DONE.

    RESULT
        Selected palette index, or -1 on cancellation.

    NOTES
        The requester changes colors in the selected screen's viewport.

    SEE ALSO
        rtPaletteRequestA()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtPaletteRequestA(title, reqinfo, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtPaletteRequest */
