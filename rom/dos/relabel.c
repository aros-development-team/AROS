/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(LONG, Relabel,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, drive, D1),
        AROS_LHA(CONST_STRPTR, newname, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 120, Dos)

/*  FUNCTION
        Change names of a volume.

    INPUTS
        drive   - The name of the device to rename (including the ':').
        newname - The new name for the device (without the ':').

    RESULT
        A boolean telling whether the name change was successful or not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    if (!getdevpacketinfo(DOSBase, drive, newname, &phs))
        return DOSFALSE;
 
    status = dopacket1(DOSBase, NULL, phs.port, ACTION_RENAME_DISK, phs.name);

    freepacketinfo(DOSBase, &phs);
    
    return status;
 
    AROS_LIBFUNC_EXIT
} /* Relabel */
