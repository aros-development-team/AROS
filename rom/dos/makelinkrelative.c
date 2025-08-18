/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Create a hard or soft link.
*/
#include <dos/dosextens.h>
#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

        AROS_LH4(LONG, MakeLinkRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name, D2),
        AROS_LHA(SIPTR,        dest, D3),
        AROS_LHA(LONG,         soft, D4),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 237, Dos)

/*  FUNCTION
        MakeLink() will create a link between two files or directories.
        A link is a filesystem object that refers to another file.

        A soft link refers to another file or directory by name, and is
        resolved by the filesystem and the caller. Soft links are not
        restricted to the same volume and the target does not have to exist.

        A hard link refers to another file by the location on a disk, and
        is resolved by the filesystem. Hard links are restricted to files
        or directories on the same volume.

    INPUTS
        name - The name of the link to create
        dest - If 'soft' is TRUE this must be a filename; if it is FALSE a lock
               pointing to the file to be hard-linked must be provided
        soft - TRUE, if a soft link is to be created, FALSE for a hard link

    RESULT
        boolean - DOSTRUE or DOSFALSE. On error, IoErr() will contain more
        information.

    NOTES
        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status;

    status = DOSFALSE;
    if (getpacketinfo(DOSBase, lock, name, &phs))
    {
        status = dopacket4(DOSBase, NULL, phs.port, ACTION_MAKE_LINK,
            phs.lock, phs.name, (SIPTR)dest, soft ? LINK_SOFT: LINK_HARD);
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* MakeLinkRelative */
