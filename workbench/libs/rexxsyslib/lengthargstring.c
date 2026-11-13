/*
        Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
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
        Returns the stored byte length of a Rexx argument string. Unlike
        strlen(), this function does not stop at an embedded NUL byte.

    INPUTS
        argstring - Non-NULL pointer to the data of a valid RexxArg,
                    such as one returned by CreateArgstring().

    RESULT
        Stored byte length, excluding the terminating NUL byte.

    NOTES
        The argument must be a valid Rexx argument string, not an
        ordinary NUL-terminated C string.

    SEE ALSO
        CreateArgstring(), DeleteArgstring()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    static struct RexxArg dummy;
    struct RexxArg *ra;
  
    ra = (struct RexxArg *)(argstring - ((void *)dummy.ra_Buff - (void *)&dummy));
  
    ReturnInt("LengthArgstring", ULONG, ra->ra_Length);
    AROS_LIBFUNC_EXIT
} /* LengthArgstring */
