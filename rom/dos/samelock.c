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

        AROS_LH2(LONG, SameLock,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock1, D1),
        AROS_LHA(BPTR, lock2, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 70, Dos)

/*  FUNCTION
        Compares two locks.

    INPUTS
        lock1, lock2 - locks to compare

    RESULT
        LOCK_SAME        - locks points to the same object
        LOCK_SAME_VOLUME - locks are on the same volume
        LOCK_DIFFERENT   - locks are different

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct FileLock *fl1;
    struct FileLock *fl2;
    LONG status;
    SIPTR res;

    if(!SameDevice(lock1, lock2))
        return LOCK_DIFFERENT;
        
    fl1 = (struct FileLock *)BADDR(lock1);
    fl2 = (struct FileLock *)BADDR(lock2);

    status = dopacket2(DOSBase, &res, fl1->fl_Task, ACTION_SAME_LOCK, lock1, lock2);
    if (status)
        return LOCK_SAME;
    if (res == ERROR_ACTION_NOT_KNOWN) {
        SetIoErr(0);
        if (fl1->fl_Volume == fl2->fl_Volume && fl1->fl_Key == fl2->fl_Key)
            return LOCK_SAME;
        if (fl1->fl_Volume == fl2->fl_Volume)
            return LOCK_SAME_VOLUME;
    }
    return LOCK_DIFFERENT;

    AROS_LIBFUNC_EXIT
} /* SameLock */
