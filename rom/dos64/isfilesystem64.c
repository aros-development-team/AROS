/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Query whether a filesystem supports 64-bit packets.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH1(LONG, IsFileSystem64,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 23, Dos64)

/*  FUNCTION
        Query whether the filesystem behind an open file understands
        64-bit packets, i.e. whether the 64-bit dos64.library
        operations on it are handled natively rather than through
        32-bit delegation.

        Applications may use this to cache the capability per file and
        skip 64-bit attempts on filesystems that cannot service them.

    INPUTS
        file - filehandle to query

    RESULT
        DOSTRUE when the filesystem handles 64-bit packets, DOSFALSE
        otherwise.

    NOTES
        The query is answered with (position-neutral) probe packets;
        no file state is modified.

    EXAMPLE

    BUGS

    SEE ALSO
        Seek64(), GetFilePosition64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh = BADDR(file);
    SIPTR err = 0;
    QUAD  ret;

    if (fh == NULL || fh->fh_Type == BNULL)
        return DOSFALSE;

#if (__WORDSIZE == 64)
    /* MorphOS style: a null seek returns the current position */
    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_SEEK64,
                        fh->fh_Arg1, 0, OFFSET_CURRENT, 0, 0, &err);
    if (!dos64_UnsupportedPkt(ret, err))
        return DOSTRUE;

    /* AmigaOS 4 style position query */
    ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_GET_FILE_POSITION64,
                        fh->fh_Arg1, 0, 0, 0, 0, &err);
#else
    ret = dos64_SendPkt64OS4(DOS64Base, fh->fh_Type, ACTION_GET_FILE_POSITION64,
                             fh->fh_Arg1, 0, 0, &err);
#endif
    if (!dos64_UnsupportedPkt(ret, err))
        return DOSTRUE;

    return DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* IsFileSystem64 */
