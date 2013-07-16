/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of BuildEasyRequestArgs() (intuition.library)
    Lang: english
*/
#include <stdarg.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <exec/memory.h>

	struct Window * BuildEasyRequest (

/*  SYNOPSIS */
	struct Window	  * RefWindow,
	struct EasyStruct * easyStruct,
	ULONG		    IDCMP,
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	intuition.library/BuildEasyRequestArgs()

    INTERNALS

*****************************************************************************/
{
    va_list args;
    struct Window * rc;
    const char *ptr;
    int argcnt = 0;
    IPTR *argtable = NULL;

    for (ptr = easyStruct->es_TextFormat; *ptr; ptr++)
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

    for (ptr = easyStruct->es_GadgetFormat; *ptr; ptr++)
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
    	va_start (args, IDCMP);

    	int i;

    	argtable = AllocVec(sizeof(IPTR)*argcnt, MEMF_PUBLIC);

    	for (i=0; i < argcnt; i++)
    		argtable[i] = va_arg(args, IPTR);

    	va_end (args);
    }

    rc = BuildEasyRequestArgs (RefWindow, easyStruct, IDCMP, (APTR)argtable);

    FreeVec(argtable);

    return rc;
} /* BuildEasyRequest */
