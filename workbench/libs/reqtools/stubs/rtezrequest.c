/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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
