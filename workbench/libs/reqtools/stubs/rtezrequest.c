/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <stdarg.h>
#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <exec/memory.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	ULONG rtEZRequest (

/*  SYNOPSIS */
	const char *bodyfmt,
	const char *gadfmt,
	struct rtReqInfo *reqinfo,
	struct TagItem *taglist,
	...)

/*  FUNCTION
        Display a formatted requester and obtain a user response.

    INPUTS
        bodyfmt - Message format string.
        gadfmt  - Gadget-label format string; separate choices with '|'.
        reqinfo - Optional requester information.
        taglist - Optional TagItem array controlling the requester.
        ...     - Formatting arguments for bodyfmt followed by gadfmt.

    RESULT
        Response code from rtEZRequestA(); the rightmost choice returns
        zero. Other choices return their assigned positive values, or
        an IDCMP value may be returned.

    NOTES
        Formatting uses RawDoFmt conventions. This wrapper counts percent
        conversions in both format strings to collect formatting arguments;
        '%%' is treated as a literal percent sign.

    BUGS
        If allocation of the temporary argument array fails, the wrapper
        writes through the NULL pointer when formatting arguments exist.

    SEE ALSO
        rtEZRequestA(), rtEZRequestTags()

***************************************************************************/
{
    va_list args;
    ULONG   rc;

    const char *ptr;
    int argcnt = 0;
    IPTR *argtable = NULL;

    for (ptr = bodyfmt; *ptr; ptr++)
    {
    	if (*ptr == '%')
    	{
    		if (ptr[1] == '%')
    		{
    			ptr++;
    			continue;
    		}

    		argcnt++;
    	}
    }

    for (ptr = gadfmt; *ptr; ptr++)
    {
    	if (*ptr == '%')
    	{
    		if (ptr[1] == '%')
    		{
    			ptr++;
    			continue;
    		}

    		argcnt++;
    	}
    }

    if (argcnt)
    {
    	va_start (args, taglist);

    	int i;

    	argtable = AllocVec(sizeof(IPTR)*argcnt, MEMF_PUBLIC);

    	for (i=0; i < argcnt; i++)
    		argtable[i] = va_arg(args, IPTR);

    	va_end (args);
    }

    rc = rtEZRequestA(bodyfmt, gadfmt, reqinfo, argtable, taglist);

    if (argtable)
    	FreeVec(argtable);

    return rc;

} /* rtEZRequest */
