/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <exec/memory.h>
	LONG EasyRequest (

/*  SYNOPSIS */
	struct Window	  * window,
	struct EasyStruct * easyStruct,
	ULONG		  * idcmpPtr,
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    va_list args;
    LONG    rc;
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
    	va_start (args, idcmpPtr);

    	int i;

    	argtable = AllocVec(sizeof(IPTR)*argcnt, MEMF_PUBLIC);

    	for (i=0; i < argcnt; i++)
    		argtable[i] = va_arg(args, IPTR);

    	va_end (args);
    }

    rc = EasyRequestArgs (window, easyStruct, idcmpPtr, (APTR)argtable);

    FreeVec(argtable);

    return rc;
} /* EasyRequest */
