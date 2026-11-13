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
	ULONG rtGetLong (

/*  SYNOPSIS */
	ULONG *longptr,
	char *title,
	struct rtReqInfo *reqinfo,
	Tag tag1, 
	...)

/*  FUNCTION
        Request a signed 32-bit integer.

    INPUTS
        longptr - Address of the value to edit.
        title   - Requester window title.
        reqinfo - Optional requester information.
        tag1    - First requester tag; terminate with TAG_DONE.

    RESULT
        Response code from rtGetLongA().

    NOTES
        An empty numeric field can cause the leftmost gadget to return
        zero without changing *longptr. See rtGetLongA() for the
        complete response and validation contract.

    SEE ALSO
        rtGetLongA()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtGetLongA(longptr, title, reqinfo, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtGetLong */
