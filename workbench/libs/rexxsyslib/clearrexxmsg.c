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

	AROS_LH2(VOID, ClearRexxMsg,

/*  SYNOPSIS */
	AROS_LHA(struct RexxMsg *, msgptr, A0),
	AROS_LHA(ULONG           , count , D0),

/*  LOCATION */
	struct Library *, RexxSysBase, 26, RexxSys)

/*  FUNCTION
        This function will clear a specified number of arguments by calling
        DeleteArgstring on them.

    INPUTS
        msgptr - RexxMsg to clear the arguments from
        count  - The number of arguments in the message to clear

    RESULT
        void

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
         FillRexxMsg(), DeleteArgstring()

    INTERNALS


*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    ULONG i;
    
    for (i = 0; i < count; i++)
    {
	if (msgptr->rm_Args[i] != 0)
	{
	    DeleteArgstring(RXARG(msgptr,i));
	    msgptr->rm_Args[i] = 0;
	}
    }
    
    ReturnVoid("ClearRexxMsg");
    AROS_LIBFUNC_EXIT
} /* ClearRexxMsg */
