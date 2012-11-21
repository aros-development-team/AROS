/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2I(struct DosList *, NextDosEntry,

/*  SYNOPSIS */
        AROS_LHA(struct DosList *, dlist, D1),
        AROS_LHA(ULONG           , flags, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 115, Dos)

/*  FUNCTION
        Looks for the next dos list entry with the right type. The list
        must be locked for this.

    INPUTS
        dlist - the value given by LockDosList() or the last call to
                FindDosEntry().
        flags - the same flags as given to LockDosList() or a subset
                of them.

    RESULT
        Pointer to dos list entry found or NULL if the are no more entries.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    static const ULONG flagarray[]=
    { 0, LDF_DEVICES, LDF_ASSIGNS, LDF_VOLUMES, LDF_ASSIGNS, LDF_ASSIGNS };

    /* Follow the list */
    for(;;)
    {
        /* Get next entry. Return NULL if there is none. */
        dlist = BADDR(dlist->dol_Next);
        if(dlist==NULL)
            return NULL;

        /* Check type */
        if(flags&flagarray[dlist->dol_Type+1])
            return dlist;
    }
    AROS_LIBFUNC_EXIT
} /* NextDosEntry */
