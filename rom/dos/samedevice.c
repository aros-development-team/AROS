/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, SameDevice,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock1, D1),
        AROS_LHA(BPTR, lock2, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 164, Dos)

/*  FUNCTION
        Checks if two locks are on the same device.

    INPUTS
        lock1, lock2 - locks to compare

    RESULT
        DOSTRUE when locks are on the same device

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct FileLock *fl1, *fl2;
    
    if (lock1 == BNULL || lock2 == BNULL)
        return DOSFALSE;
        
    fl1 = (struct FileLock *)BADDR(lock1);
    fl2 = (struct FileLock *)BADDR(lock2);

    if (fl1->fl_Volume == fl2->fl_Volume && fl1->fl_Task == fl2->fl_Task)
        return DOSTRUE;

    return DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* SameDevice */
