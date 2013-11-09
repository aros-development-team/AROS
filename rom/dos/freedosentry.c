/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(void, FreeDosEntry,

/*  SYNOPSIS */
        AROS_LHA(struct DosList *, dlist, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 117, Dos)

/*  FUNCTION
        Free a dos list entry created with MakeDosEntry().

    INPUTS
        dlist - pointer to dos list entry. May be NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (dlist != NULL)
    {
        /* It's important to free dol_Name here due to BSTR compatibility.
           See MakeDosEntry() */
        FreeVec(BADDR(dlist->dol_Name));
        FreeVec(dlist);
    }

    AROS_LIBFUNC_EXIT
} /* FreeDosEntry */
