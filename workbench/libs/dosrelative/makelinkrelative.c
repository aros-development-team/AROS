/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Create a hard or soft link relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH4(LONG, MakeLinkRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name, D2),
        AROS_LHA(SIPTR,        dest, D3),
        AROS_LHA(LONG,         soft, D4),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 15, DosRelative)

/*  FUNCTION
        MakeLinkRelative() will create a link between two files or
        directories, resolving a relative link name against the given
        directory lock instead of the current directory of the calling
        process. A link is a filesystem object that refers to another
        file.

        A soft link refers to another file or directory by name, and is
        resolved by the filesystem and the caller. Soft links are not
        restricted to the same volume and the target does not have to
        exist. The target name is stored as given; a relative target is
        later resolved against the directory holding the link.

        A hard link refers to another file by the location on a disk,
        and is resolved by the filesystem. Hard links are restricted to
        files or directories on the same volume.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/MakeLink().

    INPUTS
        lock - directory lock to resolve a relative link name against,
               or NULL for the current directory
        name - The name of the link to create
        dest - If 'soft' is TRUE this must be a filename; if it is FALSE
               a lock pointing to the file to be hard-linked must be
               provided
        soft - TRUE, if a soft link is to be created, FALSE for a hard
               link

    RESULT
        boolean - DOSTRUE or DOSFALSE. On error, IoErr() will contain
        more information.

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/MakeLink(), dos.library/ReadLink()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[MakeLinkRelative] lock=0x%p '%s' soft=%d\n", BADDR(lock), name, soft));

    if (name == NULL)
        return DOSFALSE;

    if (lock == BNULL || dosrel_IsAbsolute(name))
        return MakeLink(name, dest, soft);

    return dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_MAKE_LINK,
                              dest, soft ? LINK_SOFT : LINK_HARD, TRUE);

    AROS_LIBFUNC_EXIT
} /* MakeLinkRelative */
