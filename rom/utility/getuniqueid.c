/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/31 12:58:12  aros
    Merged in/modified for FreeBSD.

    Desc:
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <clib/utility_protos.h>

        __AROS_LH0(ULONG, GetUniqueID,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 45, Utility)

/*  FUNCTION
        Returns a unique id that is different from any other id that is
        obtained from this function call.

    INPUTS

    RESULT
        an unsigned long id

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Calls Disable()/Enable() to guarentee uniqueness.

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        17-08-96    iaint   Reimplemented. CVS lost my old one. Well I did.

*****************************************************************************/
{
    __AROS_FUNC_INIT

    struct ExecBase *SysBase = UtilityBase->ub_SysBase;

    ULONG ret;

    Disable();

    ret = ++UtilityBase->ub_LastID;

    Enable();

    return ret;

    __AROS_FUNC_EXIT
} /* GetUniqueID */
