/*
        Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

	AROS_LH3(BOOL, FillRexxMsg,

/*  SYNOPSIS */
	AROS_LHA(struct RexxMsg *, msgptr, A0),
	AROS_LHA(ULONG           , count , D0),
	AROS_LHA(ULONG           , mask  , D1),

/*  LOCATION */
	struct Library *, RexxSysBase, 27, RexxSys)

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
    AROS_LIBFUNC_INIT
    STRPTR args[16];
    char number[20];
    ULONG i, j;
    
    for (i = 0; i < count; i++)
    {
	/* Is argument i an integer ? */
	if (mask & (1<<i))
	{
	    /* Convert int to string */
	    sprintf(number, "%ld", (LONG)msgptr->rm_Args[i]);
	    args[i] = (STRPTR)CreateArgstring(number, strlen(number));
	    
	    /* Clean up if error in CreateArgstring */
	    if (args[i] == NULL)
	    {
	        for (j = 0; j < i; j++)
		    if (args[j] != NULL) DeleteArgstring((UBYTE *)args[j]);
		ReturnBool("FillRexxMsg", FALSE);
	    }
	}
	else
	{
	    /* CreateArgstring with null terminated string if pointer is not null */
	    if (msgptr->rm_Args[i] == NULL) args[i] = NULL;
	    else
	    {
		args[i] = (STRPTR)CreateArgstring(msgptr->rm_Args[i], strlen(msgptr->rm_Args[i]));
	    
		/* Clean up if error in CreateArgstring */
		if (args[i] == NULL)
		{
		    for (j = 0; j < i; j++)
		        if (args[j] != NULL) DeleteArgstring((UBYTE *)args[j]);
		    ReturnBool("FillRexxMsg", FALSE);
		}
	    }
	}
    }
    
    CopyMem(args, msgptr->rm_Args, count * sizeof(STRPTR));
    ReturnBool("FillRexxMsg", TRUE);
    AROS_LIBFUNC_EXIT
} /* FillRexxMsg */
