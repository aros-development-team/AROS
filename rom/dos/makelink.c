/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a hard or soft link.
    Lang: English
*/
#include <dos/dosextens.h>
#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

        AROS_LH3(LONG, MakeLink,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(SIPTR,   dest, D2),
        AROS_LHA(LONG  , soft, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 74, Dos)

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

    EXAMPLE

    BUGS

    SEE ALSO
        ReadLink()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status;

    status = DOSFALSE;
    if (getpacketinfo(DOSBase, name, &phs))
    {
        status = dopacket4(DOSBase, NULL, phs.port, ACTION_MAKE_LINK,
            phs.lock, phs.name, (SIPTR)dest, soft ? LINK_SOFT: LINK_HARD);
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* MakeLink */
