/*
        Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "rexxsyslib_intern.h"

#include <stddef.h>

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

	AROS_LH1(VOID, DeleteArgstring,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, argstring, A0),

/*  LOCATION */
	struct RxsLib *, RexxSysBase, 22, RexxSys)

/*  FUNCTION
        Deletes a RexxArg structure previously created with CreateArgstring

    INPUTS
        Pointer to the string part of the RexxArg structure returned from
        CreateArgstring

    RESULT
        void

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CreateArgstring()

    INTERNALS


*****************************************************************************/
{
    AROS_LIBFUNC_INIT
 
    struct RexxArg *ra;
  
    ra = (struct RexxArg *)(argstring - offsetof(struct RexxArg, ra_Buff));
    FreeMem(ra, ra->ra_Size);
  
    ReturnVoid("DeleteArgstring");
    AROS_LIBFUNC_EXIT
} /* DeleteArgstring */
