/*
        Copyright (C) 1995-2002, The AROS Development Team. All rights reserved.

    Desc:
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
        Releases non-NULL Rexx argument strings in the first count entries
        of msgptr->rm_Args and sets those entries to zero.

    INPUTS
        msgptr - RexxMsg containing argument strings to release.
        count  - Number of entries, starting at index 0; must not exceed
                 MAXRMARG.

    RESULT
        None.

    NOTES
        Every non-NULL entry in the selected range must be a valid,
        allocated Rexx argument string. Do not use this function on
        unconverted string pointers or integer values prepared for
        FillRexxMsg(). Entries beyond count are not changed.

    BUGS
        The implementation does not validate count against the rm_Args
        array capacity. Larger values can access memory outside the array.

    SEE ALSO
        FillRexxMsg(), DeleteArgstring(), DeleteRexxMsg()

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
