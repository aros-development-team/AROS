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
    
    ULONG i;
    
    for (i = 0; i < count; i++)
        if (msgptr->rm_Args[i] != NULL)
	    DeleteArgstring(msgptr->rm_Args[i]);
    
    ReturnVoid("ClearRexxMsg");
    AROS_LIBFUNC_EXIT
} /* ClearRexxMsg */
