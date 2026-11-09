/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Examine an object by name with 64-bit sizes.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

static BOOL dos64_SameName(CONST_STRPTR a, CONST_STRPTR b)
{
    for (;;)
    {
        UBYTE ca = *a++, cb = *b++;

        if ((ca >= 'a' && ca <= 'z') || (ca >= 0xe0 && ca <= 0xfe && ca != 0xf7))
            ca -= 0x20;
        if ((cb >= 'a' && cb <= 'z') || (cb >= 0xe0 && cb <= 0xfe && cb != 0xf7))
            cb -= 0x20;
        if (ca != cb)
            return FALSE;
        if (ca == 0)
            return TRUE;
    }
}

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH3(LONG, ExamineName64,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR,             name, D1),
        AROS_LHA(struct FileInfoBlock *,   fib,  D2),
        AROS_LHA(struct TagItem *,         tags, D3),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 24, Dos64)

/*  FUNCTION
        Fill a 64-bit FileInfoBlock with information about the named
        object.

        A file that is open for writing cannot be locked. When the lock
        fails with ERROR_OBJECT_IN_USE the entry is read from the
        directory that contains it, so the size of a file that is being
        written is available too.

    INPUTS
        name - path of the object, relative to the current directory
        fib  - FileInfoBlock to fill, allocated with
               AllocDosObject64(DOS64_FIB, NULL)
        tags - future extensions, pass NULL

    RESULT
        Boolean success indicator. On failure IoErr() gives additional
        information.

    NOTES
        This is an extension of dos64.library; it needs the version that
        has it.

    EXAMPLE

    BUGS

    SEE ALSO
        Examine64(), ExNext64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR lock;
    LONG ret, err;
    CONST_STRPTR file;
    STRPTR parent;
    LONG len;
    BOOL found = FALSE;

    lock = Lock(name, SHARED_LOCK);
    if (lock != BNULL)
    {
        ret = Examine64(lock, fib, tags);
        err = IoErr();
        UnLock(lock);
        if (!ret)
            SetIoErr(err);
        return ret;
    }

    if (IoErr() != ERROR_OBJECT_IN_USE)
        return DOSFALSE;

    /* the entry of the directory that holds the object */
    file = FilePart(name);
    if (*file == 0)
    {
        SetIoErr(ERROR_OBJECT_IN_USE);
        return DOSFALSE;
    }
    len = file - name;
    if (len > 1 && name[len - 1] == '/' && name[len - 2] != ':' && name[len - 2] != '/')
        len--;

    parent = AllocVec(len + 1, MEMF_PUBLIC);
    if (parent == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }
    CopyMem(name, parent, len);
    parent[len] = 0;
    lock = Lock(parent, SHARED_LOCK);
    FreeVec(parent);
    if (lock == BNULL)
        return DOSFALSE;

    if (Examine64(lock, fib, tags))
    {
        while (ExNext64(lock, fib, tags))
        {
            if (dos64_SameName(fib->fib_FileName, file))
            {
                found = TRUE;
                break;
            }
        }
    }
    UnLock(lock);

    if (!found)
        SetIoErr(ERROR_OBJECT_IN_USE);
    return found ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* ExamineName64 */
