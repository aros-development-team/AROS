/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <stdio.h>
#include <string.h>
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
	struct RxsLib *, RexxSysBase, 27, RexxSys)

/*  FUNCTION
        This function will convert the value(s) provided in rm_Args of the
        RexxMsg. The input can be either a string or a number.

    INPUTS
        msgptr - RexxMsg to create the RexxArgs for.
        count  - The number of ARGs in the rm_Args structure field that is
                 filled with a value and has to be converted.
        mask   - Bit 0-count from this mask indicate wether the value in
                 rm_Args is a string or a number. When the bit is cleared the
                 value is a pointer to a string. When it is set it is treated
                 as a signed number.

    RESULT
        Returns TRUE if succeeded, FALSE otherwise. When FALSE is returned all
        memory already allocated will be Freed before returning.

    NOTES

    EXAMPLE
        This code will convert a string and a number to RexxArgs:

        struct RexxMsg *rm;

        ...

        rm->rm_Args[0] = "Test";
        rm->rm_Args[1] = (UBYTE *)5;

        if (!FillRexxMsg(rm, 2, 1<<1))
        ...

    BUGS

    SEE ALSO
        ClearRexxMsg(), CreateRexxMsg(), CreateArgstring()

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
	    sprintf(number, "%ld", (long)msgptr->rm_Args[i]);
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
	    if (msgptr->rm_Args[i] == 0) args[i] = NULL;
	    else
	    {
		args[i] = (STRPTR)CreateArgstring(RXARG(msgptr,i), strlen(RXARG(msgptr,i)));
	    
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
