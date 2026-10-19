/*
        Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
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
        Releases storage for a Rexx argument string using the allocation
        information associated with its data pointer.

    INPUTS
        argstring - Non-NULL pointer to the data of an allocated RexxArg,
                    such as one returned by CreateArgstring(). It must not
                    have been freed already.

    RESULT
        None.

    NOTES
        Do not pass an ordinary C string or a pointer into the middle of
        an argument string. The pointer becomes invalid after this call.

    SEE ALSO
        CreateArgstring(), LengthArgstring(), ClearRexxMsg()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
 
    struct RexxArg *ra;
  
    ra = (struct RexxArg *)(argstring - offsetof(struct RexxArg, ra_Buff));
    FreeMem(ra, ra->ra_Size);
  
    ReturnVoid("DeleteArgstring");
    AROS_LIBFUNC_EXIT
} /* DeleteArgstring */
