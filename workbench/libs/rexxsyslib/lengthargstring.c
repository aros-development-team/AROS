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

	AROS_LH1(ULONG, LengthArgstring,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, argstring, A0),

/*  LOCATION */
	struct Library *, RexxSysBase, 23, RexxSys)

/*  FUNCTION
        This will return the length of a string created with CreateArgstring

    INPUTS
        argstring - Pointer to the string part of a RexxArg structure returned
                    from CreateArgstring

    RESULT
        length of the argstring

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CreateArgstring()

    INTERNALS


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    static struct RexxArg dummy;
    struct RexxArg *ra;
  
    ra = (struct RexxArg *)(argstring - ((void *)dummy.ra_Buff - (void *)&dummy));
  
    ReturnInt("LengthArgstring", ULONG, ra->ra_Length);
    AROS_LIBFUNC_EXIT
} /* LengthArgstring */
