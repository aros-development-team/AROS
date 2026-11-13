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
	ULONG rtEZRequestTags (

/*  SYNOPSIS */
	const char *bodyfmt,
	const char *gadfmt,
	struct rtReqInfo *reqinfo,
	APTR argarray,
	Tag tag1, 
	...)

/*  FUNCTION
        Display a formatted requester using a variadic tag list.

    INPUTS
        bodyfmt - Message format string.
        gadfmt  - Gadget-label format string.
        reqinfo - Optional requester information.
        argarray - Formatting arguments for bodyfmt and gadfmt.
        tag1    - First requester tag; terminate with TAG_DONE.

    RESULT
        Response code from rtEZRequestA().

    NOTES
        This is the tag-varargs form of rtEZRequestA(). Formatting
        arguments are supplied separately through argarray.

    SEE ALSO
        rtEZRequestA(), rtEZRequest()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtEZRequestA(bodyfmt, gadfmt, reqinfo, argarray, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtEZRequestTags */
