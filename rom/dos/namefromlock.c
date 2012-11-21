/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Retrieve the full pathname from a lock.
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(BOOL, NameFromLock,

/*  SYNOPSIS */
        AROS_LHA(BPTR,   lock,   D1),
        AROS_LHA(STRPTR, buffer, D2),
        AROS_LHA(LONG,   length, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 67, Dos)

/*  FUNCTION
        Get the full path name associated with a lock to a file or
        directory into a user supplied buffer.
        If the lock is zero the buffer will be filled with "SYS:".

    INPUTS
        lock   - Lock to file or directory or 0.
        buffer - Buffer to fill. Contains a NUL terminated string if
                 all went well.
        length - Size of the buffer in bytes.

    RESULT
        !=0 if all went well, 0 in case of an error. IoErr() will
        give additional information in that case.

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    /*
     * We could simply call namefrom_internal() with our lock. However this
     * causes prolems with Mount on SFS. Mount with a pattern (e. g. Mount DEVS:DOSDrivers/#?)
     * enters endless loop examining the same first file all times.
     * The problem occurs because Mount calls NameFromLock() on AnchorPath's ap_Current->an_Lock().
     * This results in calling Examine() on this lock.
     * SFS, in its turn, uses own extended form of a lock, and stores some information about current
     * search position in it. Calling Examine() on this lock, even with another FIB, causes search
     * position to be reset to the beginning of the directory.
     * This is unlikely a bug in Mount, because this code perfectly works on MorphOS without any
     * modifications. This is either:
     * a) A bug in SFS itself (unlikely, should have been noticed and fixed)
     * b) Wrong implementation of our NameFromLock().
     *
     * Duplicating a lock here is a brute-force workaround for this problem. When i have more time, i'll
     * pick up my old archive with MorphOS dos.library code, and check theirs implementation.
     *                                                                  Sonic
     */
    BOOL res;
    BPTR lock2;

    if (lock == BNULL) {
        if (length > 5) {
            CopyMem("SYS:", buffer, 5);
            return DOSTRUE;
        } else {
            SetIoErr(ERROR_LINE_TOO_LONG);
            return DOSFALSE;
        }
    }

    
    lock2 = DupLock(lock);
    res = namefrom_internal(DOSBase, lock2, buffer, length);
    UnLock(lock2);

    return res;

    AROS_LIBFUNC_EXIT
}
